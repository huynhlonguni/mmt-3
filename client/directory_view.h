double last_click = 0;
bool file_popup = 0;
bool drop_popup = 0;
fs::path from_path = "";
fs::path to_path = "";
bool from_is_folder = 0;
bool copy_0_move_1 = 0;

fs::path filesystem_get_unicode_path(std::string path) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(path.c_str());
	return fs::path(wide);
}

void FileOperationPopup(nk_context *ctx) {
	static fs::path current_dir = to_path;
	static std::string selected_entry = "";
	static std::vector<FileInfo> files_list;
	static double last_files_get_time = -FILELIST_FETCH_INTERVAL_SECONDS;

	double time = GetTime();
	if (time - last_files_get_time >= FILELIST_FETCH_INTERVAL_SECONDS) { //5 seconds
		files_list = control_client.listDir(current_dir.string());
		last_files_get_time = time;
	}

	if (view_begin("Select destination", 1)) {
		nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT * 2);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_end(ctx);
		nk_select_label(ctx, "From", NK_TEXT_LEFT, 0);
		nk_select_label(ctx, from_path.string().c_str(), NK_TEXT_LEFT, 1);
		nk_select_label(ctx, "To", NK_TEXT_LEFT, 0);
		if (current_dir == "")
			nk_select_label(ctx, "Drives", NK_TEXT_LEFT, 1);
		else
			nk_select_label(ctx, current_dir.string().c_str(), NK_TEXT_LEFT, 1);
		nk_layout_row_dynamic(ctx, nk_window_get_height(ctx) - UI_LINE_HEIGHT * 5, 1);
		if (nk_group_begin(ctx, "Group", NK_WINDOW_BORDER)) {
			nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
			nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT);
			nk_layout_row_template_push_dynamic(ctx);
			nk_layout_row_template_end(ctx);

			struct nk_style_selectable selectable = ctx->style.selectable;
			ctx->style.selectable.hover = nk_style_item_color(nk_rgb(42,42,42));

			for (auto &entry: files_list) {
				switch (entry.type) {
					case ENTRY_FILE: continue; //ignore files
					case ENTRY_FOLDER: nk_image(ctx, folder_img); break;
					case ENTRY_PARENT: nk_image(ctx, parent_img); break;
					case ENTRY_DRIVE: nk_image(ctx, drive_img); break;
				}
				
				int is_selected = selected_entry == entry.name;
				struct nk_rect bounds = nk_widget_bounds(ctx);
				int is_hover = nk_widget_is_hovered(ctx);
				if (nk_select_label(ctx, entry.name.c_str(), NK_TEXT_LEFT, is_selected)) {
					if (selected_entry != entry.name)
						selected_entry = entry.name;
					if (time - last_click <= MOUSE_DOUBLE_CLICK_SECONDS && is_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
						last_click = 0;
						switch (entry.type) {
							case ENTRY_FILE:
								last_files_get_time += FILELIST_FETCH_INTERVAL_SECONDS; //to counter the latter -= operation
								break;
							case ENTRY_FOLDER:
								current_dir /= filesystem_get_unicode_path(entry.name);
								break;
							case ENTRY_PARENT:
								if (current_dir == current_dir.root_path()) current_dir = "";
								else current_dir = current_dir.parent_path();
								break;
							case ENTRY_DRIVE:
								current_dir = entry.name + ":\\";
								break;
						}
						last_files_get_time -= FILELIST_FETCH_INTERVAL_SECONDS;
						selected_entry = "";
					}
				}

			}
			ctx->style.selectable = selectable;
			nk_group_end(ctx);
		}
		nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT * 3);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT * 3);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_end(ctx);

		nk_label(ctx, "", NK_TEXT_LEFT); //Pad
		if (current_dir == "") { //Disable
			struct nk_style_button button;
			button = ctx->style.button;
			ctx->style.button.normal = nk_style_item_color(nk_rgb(40,40,40));
			ctx->style.button.hover = nk_style_item_color(nk_rgb(40,40,40));
			ctx->style.button.active = nk_style_item_color(nk_rgb(40,40,40));
			ctx->style.button.border_color = nk_rgb(60,60,60);
			ctx->style.button.text_background = nk_rgb(60,60,60);
			ctx->style.button.text_normal = nk_rgb(60,60,60);
			ctx->style.button.text_hover = nk_rgb(60,60,60);
			ctx->style.button.text_active = nk_rgb(60,60,60);
			nk_button_label(ctx, "Confirm");
			ctx->style.button = button;
		}
		else {
			if (nk_button_label(ctx, "Confirm")) {
				to_path = current_dir / from_path.filename();
				if (control_client.checkExist(from_path.string(), to_path.string())) {
					file_popup = 1;
				}
				else {
					if (copy_0_move_1)
						control_client.requestMove(from_path.string(), to_path.string(), 0);
					else
						control_client.requestCopy(from_path.string(), to_path.string(), 0);
					from_path = "";
				}
			}
		}
		nk_label(ctx, "", NK_TEXT_LEFT); //Pad
		if (nk_button_label(ctx, "Cancel")) {
			from_path = "";
		}
		nk_label(ctx, "", NK_TEXT_LEFT); //Pad

		//Popup
		if (file_popup) {
			const int popup_width = 300;
			const int popup_height = 108;
			const struct nk_rect s = {nk_window_get_width(ctx) / 2 - popup_width / 2, nk_window_get_height(ctx) / 2 - popup_height / 2, popup_width, popup_height};
			if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Replace or Skip files", NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR, s)) {
				nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 1);
				nk_label(ctx, "Destination has files of the same name.", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 3);
				if (nk_button_label(ctx, "Overwrite")) {
					file_popup = 0;
					if (copy_0_move_1)
						control_client.requestMove(from_path.string(), to_path.string(), 1);
					else
						control_client.requestCopy(from_path.string(), to_path.string(), 1);
					from_path = "";
					nk_popup_close(ctx);
				}
				if (nk_button_label(ctx, "Skip")) {
					file_popup = 0;
					if (copy_0_move_1)
						control_client.requestMove(from_path.string(), to_path.string(), 0);
					else
						control_client.requestCopy(from_path.string(), to_path.string(), 0);
					from_path = "";
					nk_popup_close(ctx);
				}
				if (nk_button_label(ctx, "Cancel")) {
					file_popup = 0;
					from_path = "";
					nk_popup_close(ctx);
				}
				nk_popup_end(ctx);
			} else file_popup = 0;
		}

	}
	else {
		from_path = "";
	}
	nk_end(ctx);
}

void DirectoryView(nk_context *ctx) {
	static fs::path current_dir = "";
	static std::string selected_entry = "";
	static std::vector<FileInfo> files_list;
	static double last_files_get_time = -FILELIST_FETCH_INTERVAL_SECONDS;

	if (last_files_get_time == -FILELIST_FETCH_INTERVAL_SECONDS) {
		current_dir = filesystem_get_unicode_path(control_client.getDefaultLocation());
	}

	if (IsFileDropped()) {
		FilePathList files_dropped = LoadDroppedFiles();
		bool valid = 0;
		if (files_dropped.count == 1) {
			fs::path file_path = filesystem_get_unicode_path(files_dropped.paths[0]);
			if (fs::is_regular_file(file_path)) {
				std::error_code error;
				if (fs::file_size(file_path, error) <= MAX_FILE_DROP_SIZE_MB * 1024 * 1024) {
					valid = 1;
					std::ifstream file_stream(file_path, std::ios::binary);
					if (!file_stream.fail()) {
						file_stream.seekg(0, std::ios::end);
						size_t length = file_stream.tellg();
						file_stream.seekg(0, std::ios::beg);
						char *buffer = new char [length];
						file_stream.read(buffer, length);
						fs::path dest_path = current_dir / file_path.filename();
						control_client.requestWrite(dest_path.string(), 1, length, buffer);
						delete[] buffer;
					}
				}
			}
		}
		if (!valid) drop_popup = 1;
		UnloadDroppedFiles(files_dropped);
	}

	double time = GetTime();
	if (time - last_files_get_time >= FILELIST_FETCH_INTERVAL_SECONDS) { //5 seconds
		files_list = control_client.listDir(current_dir.string());
		last_files_get_time = time;
	}

	if (view_begin("Files")) {
		nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 1);
		if (current_dir == "")
			nk_select_label(ctx, "Drives", NK_TEXT_LEFT, 1);
		else
			nk_select_label(ctx, current_dir.string().c_str(), NK_TEXT_LEFT, 1);

		nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_end(ctx);

		struct nk_style_selectable selectable = ctx->style.selectable;
		ctx->style.selectable.hover = nk_style_item_color(nk_rgb(42,42,42));

		for (auto &entry: files_list) {
			switch (entry.type) {
				case ENTRY_FILE: nk_image(ctx, file_img); break;
				case ENTRY_FOLDER: nk_image(ctx, folder_img); break;
				case ENTRY_PARENT: nk_image(ctx, parent_img); break;
				case ENTRY_DRIVE: nk_image(ctx, drive_img); break;
			}
			
			int is_selected = selected_entry == entry.name;
			struct nk_rect bounds = nk_widget_bounds(ctx);
			int is_hover = nk_widget_is_hovered(ctx);
			if (nk_select_label(ctx, entry.name.c_str(), NK_TEXT_LEFT, is_selected)) {
				if (selected_entry != entry.name)
					selected_entry = entry.name;
				if (time - last_click <= MOUSE_DOUBLE_CLICK_SECONDS && is_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					last_click = 0;
					switch (entry.type) {
						case ENTRY_FILE:
							last_files_get_time += FILELIST_FETCH_INTERVAL_SECONDS; //to counter the latter -= operation
							break;
						case ENTRY_FOLDER:
							current_dir /= filesystem_get_unicode_path(entry.name);
							break;
						case ENTRY_PARENT:
							if (current_dir == current_dir.root_path()) current_dir = "";
							else current_dir = current_dir.parent_path();
							break;
						case ENTRY_DRIVE:
							current_dir = entry.name + ":\\";
							break;
					}
					last_files_get_time -= FILELIST_FETCH_INTERVAL_SECONDS;
					selected_entry = "";
				}
			}
			if (entry.type == ENTRY_DRIVE) continue; //Skip drives
			if (nk_contextual_begin(ctx, 0, nk_vec2(150, 200), bounds)) {
				nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 1);
				if (nk_contextual_item_image_label(ctx, copy_img, "Copy", NK_TEXT_CENTERED)) {
					from_path = current_dir / filesystem_get_unicode_path(entry.name);
					to_path = current_dir;
					copy_0_move_1 = 0;
					from_is_folder = entry.type != ENTRY_FILE;
				}
				if (nk_contextual_item_image_label(ctx, move_img, "Move", NK_TEXT_CENTERED)) {
					from_path = current_dir / filesystem_get_unicode_path(entry.name);
					to_path = current_dir;
					copy_0_move_1 = 1;
					from_is_folder = entry.type != ENTRY_FILE;
				}
				if (nk_contextual_item_image_label(ctx, delete_img, "Delete", NK_TEXT_CENTERED)) {
					fs::path del_path = current_dir / filesystem_get_unicode_path(entry.name);
					control_client.requestDelete(del_path.string());
				}
				nk_contextual_end(ctx);
			}

		}
		ctx->style.selectable = selectable;

		if (drop_popup) {
			const int popup_width = 400;
			const int popup_height = 100;
			const struct nk_rect s = {nk_window_get_width(ctx) / 2 - popup_width / 2, nk_window_get_height(ctx) / 2 - popup_height / 2, popup_width, popup_height};
			if (nk_popup_begin(ctx, NK_POPUP_STATIC, "File drop not allowed", NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE, s)) {
				nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT - 10, 1);
				nk_label(ctx, "You can only drop one file at a time", NK_TEXT_CENTERED);
				nk_label(ctx, TextFormat("File dropped can be up to %dMB in size", MAX_FILE_DROP_SIZE_MB), NK_TEXT_CENTERED);
				nk_popup_end(ctx);
			} else drop_popup = 0;
		}
	}
	else {
		current_view = VIEW_NONE;
	}
	nk_end(ctx);
}