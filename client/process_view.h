void ProcessesView(nk_context *ctx, char type) {
	static std::vector<ProcessInfo> processes;
	static double last_processes_get_time = -PROCESS_FETCH_INTERVAL_SECONDS;

	double time = GetTime();
	if (time - last_processes_get_time >= PROCESS_FETCH_INTERVAL_SECONDS) { //5 seconds
		processes = control_client.getProcesses();
		last_processes_get_time = time;
	}

	const int pid_size = 60;
	const int button_size = 130;
	const char *window_name[] = {"Procceses", "Applications"};
	if (view_begin(window_name[type])) {
		//Header
		nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_static(ctx, pid_size);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_push_static(ctx, button_size * 3);
		nk_layout_row_template_end(ctx);

		nk_label(ctx, "PID", NK_TEXT_CENTERED);
		nk_label(ctx, "Executable", NK_TEXT_LEFT);
		nk_label(ctx, "Action", NK_TEXT_LEFT);

		//Content
		nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_static(ctx, pid_size); //PID fixed width of 50
		nk_layout_row_template_push_dynamic(ctx); //Name can grow or shrink
		nk_layout_row_template_push_static(ctx, button_size); //Button
		nk_layout_row_template_push_static(ctx, button_size); //Button
		nk_layout_row_template_push_static(ctx, button_size); //Button
		nk_layout_row_template_end(ctx);
		for (auto &process: processes) {
			if (process.type == type) {
				nk_label(ctx, TextFormat("%d", process.pid), NK_TEXT_CENTERED);
				nk_label(ctx, TextFormat("%s", process.name.c_str()), NK_TEXT_LEFT);
				if (nk_button_image_label(ctx, pause_img, "Suspend", NK_TEXT_RIGHT)) {
					control_client.suspendProcess(process.pid);
					last_processes_get_time -= PROCESS_FETCH_INTERVAL_SECONDS; //So it will update
				}
				if (nk_button_image_label(ctx, play_img, "Resume", NK_TEXT_RIGHT)) {
					control_client.resumeProcess(process.pid);
					last_processes_get_time -= PROCESS_FETCH_INTERVAL_SECONDS; //So it will update
				}
				if (nk_button_image_label(ctx, stop_img, "Terminate", NK_TEXT_RIGHT)) {
					control_client.terminateProcess(process.pid);
					last_processes_get_time -= PROCESS_FETCH_INTERVAL_SECONDS; //So it will update
				}
			}
		}
	} else {
		current_view = VIEW_NONE;
	}
	nk_end(ctx);
}