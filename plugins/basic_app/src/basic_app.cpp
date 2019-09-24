#include <basic_app/include/basic_app.h>


basic_app::basic_app() :
	opengl::glfw::imgui::ImGuiMenu(){}

IGL_INLINE void basic_app::init(opengl::glfw::Viewer *_viewer)
{
	ImGuiMenu::init(_viewer);

	if (_viewer)
	{
		//Basic (necessary) parameteres
		core_percentage_size = 0.5;
		param_type = app_utils::None;
		Max_Distortion = 5;
		Highlighted_face_color = RED_COLOR;
		Fixed_face_color = BLUE_COLOR;
		Dragged_face_color = GREEN_COLOR;
		Vertex_Energy_color = RED_COLOR;
		Dragged_vertex_color = GREEN_COLOR;
		Fixed_vertex_color = BLUE_COLOR;
		model_color = GREY_COLOR;
		text_color = WHITE_COLOR;
		mouse_mode = app_utils::VERTEX_SELECT;
		view = app_utils::Horizontal;
		IsTranslate = false;
		Highlighted_face = false;
		show_text = true;
		distortion_type = app_utils::TOTAL_DISTORTION;
		solver_type = app_utils::NEWTON;
		solverInitialized = false;
		down_mouse_x = down_mouse_y = -1;
		texture_scaling_input = texture_scaling_output = 1;

		//Solver Parameters
		solver_on = false;
		
		//Load two views
		viewer->core().viewport = Vector4f(0, 0, 640, 800);
		input_view_id = viewer->core(0).id;
		output_view_id = viewer->append_core(Vector4f(640, 0, 640, 800));
		viewer->core(output_view_id).background_color += Vector4f(0.1,0,0,0);

		//set rotarion type to 2D mode
		viewer->core(output_view_id).trackball_angle = Quaternionf::Identity();
		viewer->core(output_view_id).orthographic = true;
		viewer->core(output_view_id).set_rotation_type(ViewerCore::RotationType(2));
		
		//Update scene
		Update_view();
		viewer->core(input_view_id).align_camera_center(InputModel().V, InputModel().F);
		viewer->core(output_view_id).align_camera_center(OutputModel().V, OutputModel().F);

		viewer->core(input_view_id).is_animating = true;
		viewer->core(output_view_id).is_animating = true;

		viewer->core(input_view_id).lighting_factor = 0.2;
		viewer->core(output_view_id).lighting_factor = 0;
		
		// Initialize solver thread
		newton = make_shared<newton_solver>();
		gradient_descent = make_shared<gradient_descent_solver>();
		Solver = newton;
		totalObjective = make_shared<total_objective>();	

		//maximize window
		glfwMaximizeWindow(viewer->window);
	}
}

IGL_INLINE void basic_app::draw_viewer_menu()
{
	float w = ImGui::GetContentRegionAvailWidth();
	float p = ImGui::GetStyle().FramePadding.x;
	if (ImGui::Button("Load##Mesh", ImVec2((w - p) / 2.f, 0)))
	{
		//Load new model that has two copies
		string fname = file_dialog_open();
		if (fname.length() != 0)
		{
			stop_solver_thread();
			
			set_name_mapping(0, app_utils::filename(fname));
			viewer->load_mesh_from_file(fname.c_str());
			viewer->load_mesh_from_file(fname.c_str());
			
			initializeSolver();
			Update_view();
			viewer->core(input_view_id).align_camera_center(InputModel().V, InputModel().F);
			viewer->core(output_view_id).align_camera_center(OutputModel().V, OutputModel().F);
		}
	}
	ImGui::SameLine(0, p);
	if (ImGui::Button("Save##Mesh", ImVec2((w - p) / 2.f, 0)))
	{
		viewer->open_dialog_save_mesh();
	}
			
	ImGui::Checkbox("Highlight faces", &Highlighted_face);
	ImGui::Checkbox("Show text", &show_text);

	if ((view == Horizontal) || (view == Vertical)) {
		if(ImGui::SliderFloat("Core Size", &core_percentage_size, 0, 1, to_string(core_percentage_size).c_str(), 1)){
			int frameBufferWidth, frameBufferHeight;
			glfwGetFramebufferSize(viewer->window, &frameBufferWidth, &frameBufferHeight);
			post_resize(frameBufferWidth, frameBufferHeight);
		}
	}

	if (ImGui::Combo("View", (int *)(&view), "Horizontal\0Vertical\0InputOnly\0OutputOnly\0\0")) {
		// That's how you get the current width/height of the frame buffer (for example, after the window was resized)
		int frameBufferWidth, frameBufferHeight;
		glfwGetFramebufferSize(viewer->window, &frameBufferWidth, &frameBufferHeight);
		post_resize(frameBufferWidth, frameBufferHeight);
	}

	if(ImGui::Combo("Mouse Mode", (int *)(&mouse_mode), "NONE\0FACE_SELECT\0VERTEX_SELECT\0CLEAR\0\0")) {
		if (mouse_mode == app_utils::CLEAR) {
			selected_faces.clear();
			selected_vertices.clear();
			UpdateHandles();
		}
	}

	Draw_menu_for_Solver();
	Draw_menu_for_Energies();
	Draw_menu_for_cores();
	Draw_menu_for_models();
	Draw_menu_for_colors();
	Draw_menu_for_text_results();


	follow_and_mark_selected_faces();
	Update_view();
}

IGL_INLINE void basic_app::post_resize(int w, int h)
{
	if (viewer)
	{
		if (view == app_utils::Horizontal) {
			viewer->core(input_view_id).viewport = Vector4f(0, 0, w * core_percentage_size, h);
			viewer->core(output_view_id).viewport = Vector4f(w * core_percentage_size, 0, w - (w * core_percentage_size), h);
		}
		if (view == app_utils::Vertical) {
			viewer->core(input_view_id).viewport = Vector4f(0, h * core_percentage_size, w, h - (h * core_percentage_size));
			viewer->core(output_view_id).viewport = Vector4f(0, 0, w, h * core_percentage_size);
		}
		if (view == app_utils::InputOnly) {
			viewer->core(input_view_id).viewport = Vector4f(0, 0, w, h);
			viewer->core(output_view_id).viewport = Vector4f(w + 1, h + 1, w + 2, h + 2);
		}
		if (view == app_utils::OutputOnly) {
			viewer->core(input_view_id).viewport = Vector4f(w + 1, h + 1, w + 2, h + 2);
			viewer->core(output_view_id).viewport = Vector4f(0, 0, w, h);
		}
	}
}

IGL_INLINE bool basic_app::mouse_move(int mouse_x, int mouse_y)
{
	if (!IsTranslate)
	{
		return false;
	}
	if (mouse_mode == app_utils::FACE_SELECT)
	{
		if (!selected_faces.empty())
		{
			RowVector3d face_avg_pt = get_face_avg();
			RowVector3i face = viewer->data(Model_Translate_ID).F.row(Translate_Index);
			
			Vector3f translation = app_utils::computeTranslation(mouse_x, down_mouse_x, mouse_y, down_mouse_y, face_avg_pt, viewer->core(Core_Translate_ID));
			viewer->data(Model_Translate_ID).V.row(face[0]) += translation.cast<double>();
			viewer->data(Model_Translate_ID).V.row(face[1]) += translation.cast<double>();
			viewer->data(Model_Translate_ID).V.row(face[2]) += translation.cast<double>();

			viewer->data(Model_Translate_ID).set_mesh(viewer->data(Model_Translate_ID).V, viewer->data(Model_Translate_ID).F);
			down_mouse_x = mouse_x;
			down_mouse_y = mouse_y;
			UpdateHandles();
			return true;
		}
	}
	else if (mouse_mode == app_utils::VERTEX_SELECT)
	{
		if (!selected_vertices.empty())
		{
			RowVector3d vertex_pos = viewer->data(Model_Translate_ID).V.row(Translate_Index);
			Vector3f translation = app_utils::computeTranslation(mouse_x, down_mouse_x, mouse_y, down_mouse_y, vertex_pos, viewer->core(Core_Translate_ID));
			viewer->data(Model_Translate_ID).V.row(Translate_Index) += translation.cast<double>();

			viewer->data(Model_Translate_ID).set_mesh(viewer->data(Model_Translate_ID).V, viewer->data(Model_Translate_ID).F);
			down_mouse_x = mouse_x;
			down_mouse_y = mouse_y;
			UpdateHandles();
			return true;
		}
	}
	UpdateHandles();
	return false;
}

IGL_INLINE bool basic_app::mouse_up(int button, int modifier) {
	IsTranslate = false;
	return false;
}

IGL_INLINE bool basic_app::mouse_down(int button, int modifier) {
	down_mouse_x = viewer->current_mouse_x;
	down_mouse_y = viewer->current_mouse_y;
			
	if (mouse_mode == app_utils::FACE_SELECT && button == GLFW_MOUSE_BUTTON_LEFT && modifier == 2)
	{
		//check if there faces which is selected on the left screen
		int f = pick_face(InputModel().V, InputModel().F, app_utils::InputOnly);
		if (f == -1) {
			//check if there faces which is selected on the right screen
			f = pick_face(OutputModel().V, OutputModel().F, app_utils::OutputOnly);
		}

		if (f != -1)
		{
			if (find(selected_faces.begin(), selected_faces.end(), f) != selected_faces.end())
			{
				selected_faces.erase(f);
				UpdateHandles();
			}
			else {
				selected_faces.insert(f);
				UpdateHandles();
			}
		}

	}
	else if (mouse_mode == app_utils::VERTEX_SELECT && button == GLFW_MOUSE_BUTTON_LEFT && modifier == 2)
	{
		//check if there faces which is selected on the left screen
		int v = pick_vertex(InputModel().V, InputModel().F, app_utils::InputOnly);
		if (v == -1) {
			//check if there faces which is selected on the right screen
			v = pick_vertex(OutputModel().V, OutputModel().F, app_utils::OutputOnly);
		}

		if (v != -1)
		{
			if (find(selected_vertices.begin(), selected_vertices.end(), v) != selected_vertices.end())
			{
				selected_vertices.erase(v);
				UpdateHandles();
			}
			else {
				selected_vertices.insert(v);
				UpdateHandles();
			}
					
		}
	}
	else if (mouse_mode == app_utils::FACE_SELECT && button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (!selected_faces.empty())
		{
			//check if there faces which is selected on the left screen
			int f = pick_face(InputModel().V, InputModel().F, app_utils::InputOnly);
			Model_Translate_ID = InputModelID();
			Core_Translate_ID = input_view_id;
			if (f == -1) {
				//check if there faces which is selected on the right screen
				f = pick_face(OutputModel().V, OutputModel().F, app_utils::OutputOnly);
				Model_Translate_ID = OutputModelID();
				Core_Translate_ID = output_view_id;
			}

			if (find(selected_faces.begin(), selected_faces.end(), f) != selected_faces.end())
			{
				IsTranslate = true;
				Translate_Index = f;
			}
		}
	}
	else if (mouse_mode == app_utils::VERTEX_SELECT && button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
	if (!selected_vertices.empty())
	{
		//check if there faces which is selected on the left screen
		int v = pick_vertex(InputModel().V, InputModel().F, app_utils::InputOnly);
		Model_Translate_ID = InputModelID();
		Core_Translate_ID = input_view_id;
		if (v == -1) {
			//check if there faces which is selected on the right screen
			v = pick_vertex(OutputModel().V, OutputModel().F, app_utils::OutputOnly);
			Model_Translate_ID = OutputModelID();
			Core_Translate_ID = output_view_id;
		}

		if (find(selected_vertices.begin(), selected_vertices.end(), v) != selected_vertices.end())
		{
			IsTranslate = true;
			Translate_Index = v;
		}
	}
	}

	return false;
}

IGL_INLINE bool basic_app::key_pressed(unsigned int key, int modifiers) {

	if (key == 'F' || key == 'f') {
		mouse_mode = app_utils::FACE_SELECT;
	}
	if (key == 'V' || key == 'v') {
		mouse_mode = app_utils::VERTEX_SELECT;
	}
	if (key == 'C' || key == 'c') {
		mouse_mode = app_utils::CLEAR;
		selected_faces.clear();
		selected_vertices.clear();
		UpdateHandles();
	}
	return ImGuiMenu::key_pressed(key, modifiers);
}

IGL_INLINE void basic_app::shutdown()
{
	stop_solver_thread();
	ImGuiMenu::shutdown();
}

IGL_INLINE bool basic_app::pre_draw() {
	//call parent function
	ImGuiMenu::pre_draw();

	if (Solver->progressed)
		update_mesh();

	//Update the model's faces colors in the two screens
	if (color_per_face.size()) {
		InputModel().set_colors(color_per_face);
		OutputModel().set_colors(color_per_face);
	}

	//Update the model's vertex colors in the two screens
	InputModel().point_size = 10;
	OutputModel().point_size = 10;

	InputModel().set_points(Vertices_Input, color_per_vertex);
	OutputModel().set_points(Vertices_output, color_per_vertex);

	return false;
}

void basic_app::Draw_menu_for_colors() {
	if (!ImGui::CollapsingHeader("colors", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit3("Highlighted face color", Highlighted_face_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		ImGui::ColorEdit3("Fixed face color", Fixed_face_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		ImGui::ColorEdit3("Dragged face color", Dragged_face_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		ImGui::ColorEdit3("Fixed vertex color", Fixed_vertex_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		ImGui::ColorEdit3("Dragged vertex color", Dragged_vertex_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		ImGui::ColorEdit3("Model color", model_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		ImGui::ColorEdit3("Vertex Energy color", Vertex_Energy_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		ImGui::ColorEdit4("text color", text_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
	}
}

void basic_app::Draw_menu_for_Solver() {
	if (ImGui::CollapsingHeader("Solver", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Checkbox(solver_on ? "On" : "Off", &solver_on)) {
			if (solver_on) {
				start_solver_thread();
			}
			else {
				stop_solver_thread();
			}
		}
		if (ImGui::Combo("step", (int *)(&solver_type), "Newton\0Gradient Descent\0\0")) {
			stop_solver_thread();
			if (solver_type == app_utils::NEWTON) {
				Solver = newton;
			}
			else {
				Solver = gradient_descent;
			}
			VectorXd initialguessXX = Map<const VectorXd>(OutputModel().V.leftCols(2).data(), OutputModel().V.leftCols(2).rows() * 2);
			Solver->init(totalObjective, initialguessXX);
			MatrixX3i F = OutputModel().F;
			Solver->setFlipAvoidingLineSearch(F);
			start_solver_thread();
		}

		ImGui::Combo("Dist check", (int *)(&distortion_type), "NO_DISTORTION\0AREA_DISTORTION\0LENGTH_DISTORTION\0ANGLE_DISTORTION\0TOTAL_DISTORTION\0\0");
		
		app_utils::Parametrization prev_type = param_type;
		if (ImGui::Combo("Initial Guess", (int *)(&param_type), "RANDOM\0HARMONIC\0LSCM\0ARAP\0NONE\0\0")) {
			MatrixXd initialguess;
			MatrixX3i F = OutputModel().F;
			app_utils::Parametrization temp = param_type;
			param_type = prev_type;
			if (temp == app_utils::None || !F.size()) {
				param_type = app_utils::None;
			}
			else if (app_utils::IsMesh2D(InputModel().V)) {
				if (temp == app_utils::RANDOM) {
					app_utils::random_param(InputModel().V, initialguess);
					param_type = temp;
					update_texture(initialguess);
					Update_view();
					VectorXd initialguessXX = Map<const VectorXd>(initialguess.data(), initialguess.rows() * 2);
					Solver->init(totalObjective, initialguessXX);
					Solver->setFlipAvoidingLineSearch(F);
				}
			}
			else {
				//The mesh is 3D
				if (temp == app_utils::HARMONIC) {
					app_utils::harmonic_param(InputModel().V, InputModel().F, initialguess);
				}
				if (temp == app_utils::LSCM) {
					app_utils::lscm_param(InputModel().V, InputModel().F, initialguess);
				}
				if (temp == app_utils::ARAP) {
					app_utils::ARAP_param(InputModel().V, InputModel().F, initialguess);
				}
				if (temp == app_utils::RANDOM) {
					app_utils::random_param(InputModel().V, initialguess);
				}
				param_type = temp;
				update_texture(initialguess);
				Update_view();
				VectorXd initialguessXX = Map<const VectorXd>(initialguess.data(), initialguess.rows() * 2);
				Solver->init(totalObjective, initialguessXX);
				Solver->setFlipAvoidingLineSearch(F);
			}
			
		}
		float w = ImGui::GetContentRegionAvailWidth(), p = ImGui::GetStyle().FramePadding.x;
		if (ImGui::Button("Check gradients", ImVec2((w - p) / 2.f, 0)))
		{
			checkGradients();
		}
		ImGui::SameLine(0, p);
		if (ImGui::Button("Check Hessians", ImVec2((w - p) / 2.f, 0)))
		{
			checkHessians();
		}
		
		ImGui::DragFloat("Max Distortion", &(Max_Distortion), 0.05f, 0.1f, 20.0f);
		
		ImGui::PushItemWidth(80 * menu_scaling());
		ImGui::DragFloat("shift eigen values", &(totalObjective->Shift_eigen_values), 0.07f, 0.1f, 20.0f);

		
	}
}

void basic_app::Draw_menu_for_Energies() {
	if (!ImGui::CollapsingHeader("Energies", ImGuiTreeNodeFlags_DefaultOpen))
	{
		int id = 0;
		for (auto& obj : totalObjective->objectiveList) {
			ImGui::PushID(id++);
			ImGui::Text(obj->name.c_str());
			ImGui::PushItemWidth(80 * menu_scaling());
			ImGui::DragFloat("weight", &(obj->w), 0.05f, 0.1f, 20.0f);
			ImGui::PopID();
		}
	}
}

void basic_app::Draw_menu_for_cores() {
	for (auto& core : viewer->core_list)
	{
		ImGui::PushID(core.id);
		stringstream ss;
		string name = (core.id == input_view_id) ? "Input Core" : "Output Core";
		ss << name;
		if (!ImGui::CollapsingHeader(ss.str().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			int data_id = OutputModelID();
			if (core.id == input_view_id) {
				data_id = InputModelID();
			}

			if (ImGui::Button("Center object", ImVec2(-1, 0)))
			{
				core.align_camera_center(viewer->data(data_id).V, viewer->data(data_id).F);
			}
			if (ImGui::Button("Snap canonical view", ImVec2(-1, 0)))
			{
				viewer->snap_to_canonical_quaternion();
			}

			// Zoom
			ImGui::PushItemWidth(80 * menu_scaling());
			ImGui::DragFloat("Zoom", &(core.camera_zoom), 0.05f, 0.1f, 20.0f);

			// Lightining factor
			ImGui::PushItemWidth(80 * menu_scaling());
			ImGui::DragFloat("Lighting factor", &(core.lighting_factor), 0.05f, 0.1f, 20.0f);

			// Select rotation type
			int rotation_type = static_cast<int>(core.rotation_type);
			static Quaternionf trackball_angle = Quaternionf::Identity();
			static bool orthographic = true;
			if (ImGui::Combo("Camera Type", &rotation_type, "Trackball\0Two Axes\0002D Mode\0\0"))
			{
				using RT = ViewerCore::RotationType;
				auto new_type = static_cast<RT>(rotation_type);
				if (new_type != core.rotation_type)
				{
					if (new_type == RT::ROTATION_TYPE_NO_ROTATION)
					{
						trackball_angle = core.trackball_angle;
						orthographic = core.orthographic;
						core.trackball_angle = Quaternionf::Identity();
						core.orthographic = true;
					}
					else if (core.rotation_type == RT::ROTATION_TYPE_NO_ROTATION)
					{
						core.trackball_angle = trackball_angle;
						core.orthographic = orthographic;
					}
					core.set_rotation_type(new_type);
				}
			}

			// Orthographic view
			ImGui::Checkbox("Orthographic view", &(core.orthographic));
			ImGui::PopItemWidth();
			ImGui::ColorEdit4("Background", core.background_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
		}
		ImGui::PopID();
	}
}

void basic_app::Draw_menu_for_models() {
	for (auto& data : viewer->data_list)
	{
		// Helper for setting viewport specific mesh options
		auto make_checkbox = [&](const char *label, unsigned int &option)
		{
			bool temp = option;
			bool res = ImGui::Checkbox(label, &temp);
			option = temp;
			return res;
		};

		ImGui::PushID(data.id);
		stringstream ss;

		if (data_id_to_name.count(data.id) > 0)
		{
			ss << data_id_to_name[data.id];
		}
		else
		{
			ss << "Data " << data.id;
		}

		if (!ImGui::CollapsingHeader(ss.str().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			float w = ImGui::GetContentRegionAvailWidth();
			float p = ImGui::GetStyle().FramePadding.x;

			if (data.id == InputModelID()) {
				ImGui::SliderFloat("texture", &texture_scaling_input, 0.01, 100, to_string(texture_scaling_input).c_str(), 1);
			}
			else {
				ImGui::SliderFloat("texture", &texture_scaling_output, 0.01, 100, to_string(texture_scaling_output).c_str(), 1);
			}
			

			if (ImGui::Checkbox("Face-based", &(data.face_based)))
			{
				data.dirty = MeshGL::DIRTY_ALL;
			}

			make_checkbox("Show texture", data.show_texture);
			if (ImGui::Checkbox("Invert normals", &(data.invert_normals)))
			{
				data.dirty |= MeshGL::DIRTY_NORMAL;
			}
			make_checkbox("Show overlay", data.show_overlay);
			make_checkbox("Show overlay depth", data.show_overlay_depth);
			ImGui::ColorEdit4("Line color", data.line_color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.3f);
			ImGui::DragFloat("Shininess", &(data.shininess), 0.05f, 0.0f, 100.0f);
			ImGui::PopItemWidth();

			make_checkbox("Wireframe", data.show_lines);
			make_checkbox("Fill", data.show_faces);
			ImGui::Checkbox("Show vertex labels", &(data.show_vertid));
			ImGui::Checkbox("Show faces labels", &(data.show_faceid));
		}
		ImGui::PopID();
	}
}

void basic_app::Draw_menu_for_text_results() {
	if (!show_text) {
		return;
	}

	int frameBufferWidth, frameBufferHeight;
	float shift = ImGui::GetTextLineHeightWithSpacing();
	glfwGetFramebufferSize(viewer->window, &frameBufferWidth, &frameBufferHeight);

	int w, h;
	if (view == app_utils::Horizontal) {
		w = frameBufferWidth * core_percentage_size + shift;
		h = shift;
	}
	if (view == app_utils::Vertical) {
		w = shift;
		h = frameBufferHeight - frameBufferHeight * core_percentage_size + shift;
	}
	if (view == app_utils::InputOnly) {
		w = frameBufferWidth * core_percentage_size + shift;
		h = shift;
	}
	if (view == app_utils::OutputOnly) {
		w = frameBufferWidth * core_percentage_size + shift;
		h = shift;
	}


	bool bOpened(true);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::Begin("BCKGND", &bOpened, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::SetWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::SetWindowCollapsed(false);
	ImColor c(text_color[0], text_color[1], text_color[2], 1.0f);

	//add text...
	ImGui::GetWindowDrawList()->AddText(ImVec2(w, h), c, (totalObjective->name + " energy " + to_string(totalObjective->energy_value)).c_str());
	h += shift;
	ImGui::GetWindowDrawList()->AddText(ImVec2(w, h), c, (totalObjective->name + " gradient " + to_string(totalObjective->gradient_norm)).c_str());
	h += shift;

	for (auto& obj : totalObjective->objectiveList) {
		if (obj->w != 0) {
			ImGui::GetWindowDrawList()->AddText(ImVec2(w, h), c, (obj->name + " energy " + to_string(obj->energy_value)).c_str());
			h += shift;
			ImGui::GetWindowDrawList()->AddText(ImVec2(w, h), c, (obj->name + " gradient " + to_string(obj->gradient_norm)).c_str());
			h += shift;
		}
	}

	ImGui::End();
	ImGui::PopStyleColor();
}

void basic_app::UpdateHandles() {
	vector<int> CurrHandlesInd;
	MatrixX2d CurrHandlesPosDeformed;
	CurrHandlesInd.clear();

	//First, we push each vertices index to the handles
	for (auto vi : selected_vertices) {
		CurrHandlesInd.push_back(vi);
	}
	//Then, we push each face vertices index to the handle (3 vertices)
	for (auto fi : selected_faces) {
		//Here we get the 3 vertice's index that build each face
		int v0 = OutputModel().F(fi,0);
		int v1 = OutputModel().F(fi,1);
		int v2 = OutputModel().F(fi,2);

		//check whether the handle already exist
		if (!(find(CurrHandlesInd.begin(), CurrHandlesInd.end(), v0) != CurrHandlesInd.end())){
			CurrHandlesInd.push_back(v0);
		}

		if (!(find(CurrHandlesInd.begin(), CurrHandlesInd.end(), v1) != CurrHandlesInd.end())) {
			CurrHandlesInd.push_back(v1);
		}

		if (!(find(CurrHandlesInd.begin(), CurrHandlesInd.end(), v2) != CurrHandlesInd.end())) {
			CurrHandlesInd.push_back(v2);
		}
	}
			
	//Here we update the positions for each handle
	CurrHandlesPosDeformed.resize(CurrHandlesInd.size(),2);
	int idx = 0;
	for (auto hi : CurrHandlesInd) {
		CurrHandlesPosDeformed.row(idx++) << OutputModel().V(hi, 0), OutputModel().V(hi, 1);
	}
	
	//Update texture
	update_texture(OutputModel().V);

	//Finally, we update the handles in the constraints positional object
	if (solverInitialized) {
		(*HandlesInd) = CurrHandlesInd;
		(*HandlesPosDeformed) = CurrHandlesPosDeformed;
	}
}

void basic_app::Update_view() {
	viewer->data().copy_options(viewer->core_list[0], viewer->core_list[1]);
	for (auto& core : viewer->core_list)
	{
		for (auto& data : viewer->data_list)
		{
			viewer->data(data.id).set_visible(false, core.id);
		}
	}
	InputModel().set_visible(true, input_view_id);
	OutputModel().set_visible(true, output_view_id);
}

void basic_app::follow_and_mark_selected_faces() {
	//check if there faces which is selected on the left screen
	int f = pick_face(InputModel().V, InputModel().F, app_utils::InputOnly);
	if (f == -1) {
		//check if there faces which is selected on the right screen
		f = pick_face(OutputModel().V, OutputModel().F, app_utils::OutputOnly);
	}
	
	if(InputModel().F.size()){
		//Mark the faces
		color_per_face.resize(InputModel().F.rows(), 3);
		UpdateEnergyColors();
		//Mark the fixed faces
		if (f != -1 && Highlighted_face)
		{
			color_per_face.row(f) = Highlighted_face_color.cast<double>();
		}
		for (auto fi : selected_faces) { color_per_face.row(fi) = Fixed_face_color.cast<double>(); }
		//Mark the Dragged face
		if (IsTranslate && (mouse_mode == app_utils::FACE_SELECT)) {
			color_per_face.row(Translate_Index) = Dragged_face_color.cast<double>();
		}
		
		//Mark the vertices
		int idx = 0;
		Vertices_Input.resize(selected_vertices.size(), 3);
		Vertices_output.resize(selected_vertices.size(), 3);
		color_per_vertex.resize(selected_vertices.size(), 3);
		//Mark the dragged vertex
		if (IsTranslate && (mouse_mode == app_utils::VERTEX_SELECT)) {
			Vertices_Input.resize(selected_vertices.size()+1, 3);
			Vertices_output.resize(selected_vertices.size()+1, 3);
			color_per_vertex.resize(selected_vertices.size()+1, 3);

			Vertices_Input.row(idx) = InputModel().V.row(Translate_Index);
			color_per_vertex.row(idx) = Dragged_vertex_color.cast<double>();
			Vertices_output.row(idx) = OutputModel().V.row(Translate_Index);
			idx++;
		}
				
		//Mark the fixed vertices
		for (auto vi : selected_vertices) {
			Vertices_Input.row(idx) = InputModel().V.row(vi);
			Vertices_output.row(idx) = OutputModel().V.row(vi);
			color_per_vertex.row(idx++) = Fixed_vertex_color.cast<double>();
		}
	}
}
	
void basic_app::set_name_mapping(unsigned int data_id, string name)
{
	data_id_to_name[data_id] = name;
	data_id_to_name[data_id+1] = name + " (Param.)";
}

ViewerData& basic_app::InputModel() {
	return viewer->data(InputModelID());
}

ViewerData& basic_app::OutputModel() {
	return viewer->data(OutputModelID());
}

int basic_app::InputModelID() {
	return 0;
}

int basic_app::OutputModelID() {
	return 1;
}

RowVector3d basic_app::get_face_avg() {
	RowVector3d avg; avg << 0, 0, 0;
	RowVector3i face = viewer->data(Model_Translate_ID).F.row(Translate_Index);

	avg += viewer->data(Model_Translate_ID).V.row(face[0]);
	avg += viewer->data(Model_Translate_ID).V.row(face[1]);
	avg += viewer->data(Model_Translate_ID).V.row(face[2]);
	avg /= 3;

	return avg;
}

int basic_app::pick_face(Eigen::MatrixXd& V, Eigen::MatrixXi& F, app_utils::View LR) {
	// Cast a ray in the view direction starting from the mouse position
	int core_index;
	if (LR == app_utils::OutputOnly) {
		core_index = output_view_id;
	}
	else if (LR == app_utils::InputOnly) {
		core_index = input_view_id;
	}
	double x = viewer->current_mouse_x;
	double y = viewer->core(core_index).viewport(3) - viewer->current_mouse_y;
	if (view == app_utils::Vertical) {
		y = (viewer->core(input_view_id).viewport(3) / core_percentage_size) - viewer->current_mouse_y;
	}


	Eigen::RowVector3d pt;

	Eigen::Matrix4f modelview = viewer->core(core_index).view;
	int vi = -1;

	std::vector<igl::Hit> hits;

	igl::unproject_in_mesh(Eigen::Vector2f(x, y), viewer->core(core_index).view,
		viewer->core(core_index).proj, viewer->core(core_index).viewport, V, F, pt, hits);

	int fi = -1;
	if (hits.size() > 0) {
		fi = hits[0].id;
	}
	return fi;
}

int basic_app::pick_vertex(MatrixXd& V, MatrixXi& F, app_utils::View LR) {
	// Cast a ray in the view direction starting from the mouse position
	int core_index;
	if (LR == app_utils::OutputOnly) {
		core_index = output_view_id;
	}
	else if (LR == app_utils::InputOnly) {
		core_index = input_view_id;
	}

	double x = viewer->current_mouse_x;
	double y = viewer->core(core_index).viewport(3) - viewer->current_mouse_y;
	if (view == app_utils::Vertical) {
		y = (viewer->core(input_view_id).viewport(3) / core_percentage_size) - viewer->current_mouse_y;
	}

	RowVector3d pt;

	Matrix4f modelview = viewer->core(core_index).view;
	int vi = -1;

	vector<Hit> hits;
			
	unproject_in_mesh(Vector2f(x, y), viewer->core(core_index).view,
		viewer->core(core_index).proj, viewer->core(core_index).viewport, V, F, pt, hits);

	if (hits.size() > 0) {
		int fi = hits[0].id;
		RowVector3d bc;
		bc << 1.0 - hits[0].u - hits[0].v, hits[0].u, hits[0].v;
		bc.maxCoeff(&vi);
		vi = F(fi, vi);
	}
	return vi;
}

void basic_app::update_texture(MatrixXd& V_uv) {
	MatrixXd V_uv_2D(V_uv.rows(),2);
	MatrixXd V_uv_3D(V_uv.rows(),3);
	if (V_uv.cols() == 2) {
		V_uv_2D = V_uv;
		V_uv_3D.leftCols(2) = V_uv.leftCols(2);
		V_uv_3D.rightCols(1).setZero();
	}
	else if (V_uv.cols() == 3) {
		V_uv_3D = V_uv;
		V_uv_2D = V_uv.leftCols(2);
	}

	// Plot the mesh
	InputModel().set_uv(V_uv_2D * texture_scaling_input);
	OutputModel().set_vertices(V_uv_3D);
	OutputModel().set_uv(V_uv_2D * texture_scaling_output);
	OutputModel().compute_normals();
}
	
void basic_app::checkGradients()
{
	if (!solverInitialized) {
		solver_on = false;
		return;
	}
	stop_solver_thread();
	for (auto const &objective : totalObjective->objectiveList) {
		objective->checkGradient(Solver->ext_x);
	}
	start_solver_thread();
}

void basic_app::checkHessians()
{
	if (!solverInitialized) {
		solver_on = false;
		return;
	}
	stop_solver_thread();
	for (auto const &objective : totalObjective->objectiveList) {
		objective->checkHessian(Solver->ext_x);
	}
	start_solver_thread();
}

void basic_app::start_solver_thread() {
	if(!solverInitialized){
		solver_on = false;
		return;
	}
	cout << ">> start new solver" << endl;
	solver_on = true;
	
	solver_thread = thread(&solver::run, Solver.get());
	solver_thread.detach();
}

void basic_app::stop_solver_thread() {
	solver_on = false;
	if (Solver->is_running) {
		Solver->stop();
	}
	while (Solver->is_running);
}

void basic_app::update_mesh()
{
	VectorXd X;
	Solver->get_data(X);
	MatrixXd V(X.rows() / 2, 2);
	V = Map<MatrixXd>(X.data(), X.rows() / 2, 2);
	
	if (IsTranslate) {
		Vector2d temp = OutputModel().V.row(Translate_Index);
		V.row(Translate_Index) = temp;
	}
	update_texture(V);
}

void basic_app::initializeSolver()
{
	MatrixXd V = OutputModel().V;
	MatrixX3i F = OutputModel().F;
	
	stop_solver_thread();

	if (V.rows() == 0 || F.rows() == 0)
		return;

	// initialize the energy
	auto symDirichlet = make_unique<symmetric_dirichlet>();
	symDirichlet->init_mesh(V, F);
	symDirichlet->init();
	auto oneRingAreaPreserving = make_unique<one_ring_area_preserving>();
	oneRingAreaPreserving->init_mesh(V, F);
	oneRingAreaPreserving->init();
	auto symDirichletCompositeMajorization = make_unique<symmetric_dirichlet_composite_majorization>();
	symDirichletCompositeMajorization->init_mesh(V, F);
	symDirichletCompositeMajorization->init();
	auto areaPreserving = make_unique<area_preserving>();
	areaPreserving->init_mesh(V, F);
	areaPreserving->init();
	auto anglePreserving = make_unique<angle_preserving>();
	anglePreserving->init_mesh(V, F);
	anglePreserving->init();
	auto constraintsPositional = make_shared<penalty_positional_constraints>();
	constraintsPositional->numV = V.rows();
	constraintsPositional->init();
	HandlesInd = &constraintsPositional->ConstrainedVerticesInd;
	HandlesPosDeformed = &constraintsPositional->ConstrainedVerticesPos;

	totalObjective->objectiveList.clear();
	totalObjective->objectiveList.push_back(move(areaPreserving));
	totalObjective->objectiveList.push_back(move(anglePreserving));
	totalObjective->objectiveList.push_back(move(symDirichlet));
	totalObjective->objectiveList.push_back(move(oneRingAreaPreserving));
	totalObjective->objectiveList.push_back(move(symDirichletCompositeMajorization));
	totalObjective->objectiveList.push_back(move(constraintsPositional));

	totalObjective->init();

	// initialize the solver
	MatrixXd initialguess;
	if (app_utils::IsMesh2D(InputModel().V)) {
		//the mesh is 2D
		initialguess = V;
	}
	else {
		//the mesh is 3D
		app_utils::harmonic_param(InputModel().V, InputModel().F, initialguess);
		param_type = app_utils::HARMONIC;
		update_texture(initialguess);
		Update_view();
	}
	VectorXd initialguessXX = Map<const VectorXd>(initialguess.data(), initialguess.rows() * 2);
	newton->init(totalObjective, initialguessXX);
	newton->setFlipAvoidingLineSearch(F);
	gradient_descent->init(totalObjective, initialguessXX);
	gradient_descent->setFlipAvoidingLineSearch(F);
	
	cout << "Solver is initialized!" << endl;
	solverInitialized = true;
}

void basic_app::UpdateEnergyColors() {
	int numF = OutputModel().F.rows();
	VectorXd DistortionPerFace(numF);
	DistortionPerFace.setZero();
	
	if (distortion_type == app_utils::ANGLE_DISTORTION) {	//distortion according to area preserving
		MatrixXd angle_input, angle_output, angle_ratio;
		app_utils::angle_degree(OutputModel().V, OutputModel().F, angle_output);
		app_utils::angle_degree(InputModel().V, InputModel().F, angle_input);

		// DistortionPerFace = angle_output / angle_input
		angle_ratio = angle_input.cwiseInverse().cwiseProduct(angle_output);
		//average over the vertices on each face
		DistortionPerFace = angle_ratio.rowwise().sum() / 3;
		DistortionPerFace = DistortionPerFace.cwiseAbs2().cwiseAbs2();
		// Becuase we want  DistortionPerFace to be as colse as possible to zero instead of one!
		DistortionPerFace = DistortionPerFace - VectorXd::Ones(numF);
	}
	else if (distortion_type == app_utils::LENGTH_DISTORTION) {	//distortion according to area preserving
		MatrixXd Length_output, Length_input, Length_ratio;
		igl::edge_lengths(OutputModel().V, OutputModel().F, Length_output);
		igl::edge_lengths(InputModel().V, InputModel().F, Length_input);
		// DistortionPerFace = Length_output / Length_input
		Length_ratio = Length_input.cwiseInverse().cwiseProduct(Length_output);
		//average over the vertices on each face
		DistortionPerFace = Length_ratio.rowwise().sum() / 3;
		// Becuase we want  DistortionPerFace to be as colse as possible to zero instead of one!
		DistortionPerFace = DistortionPerFace - VectorXd::Ones(numF);
	}
	else if (distortion_type == app_utils::AREA_DISTORTION) {
		//distortion according to area preserving
		VectorXd Area_output, Area_input;
		igl::doublearea(OutputModel().V, OutputModel().F, Area_output);
		igl::doublearea(InputModel().V, InputModel().F, Area_input);
		// DistortionPerFace = Area_output / Area_input
		DistortionPerFace = Area_input.cwiseInverse().cwiseProduct(Area_output);
		// Becuase we want  DistortionPerFace to be as colse as possible to zero instead of one!
		DistortionPerFace = DistortionPerFace - VectorXd::Ones(numF);
	}
	else if (distortion_type == app_utils::TOTAL_DISTORTION) {
		// calculate the distortion over all the energies
		for (auto& obj : totalObjective->objectiveList) 
			if ((obj->Efi.size() != 0) && (obj->w != 0)) 
				DistortionPerFace += obj->Efi * obj->w;
	}

	VectorXd alpha_vec = DistortionPerFace / (Max_Distortion+1e-8);
	VectorXd beta_vec = VectorXd::Ones(numF) - alpha_vec;
	MatrixXd alpha(numF, 3), beta(numF, 3);
	alpha = alpha_vec.replicate(1, 3);
	beta = beta_vec.replicate(1, 3);

	//calculate low distortion color matrix
	MatrixXd LowDistCol = model_color.cast <double>().replicate(1, numF).transpose();
	//calculate high distortion color matrix
	MatrixXd HighDistCol = Vertex_Energy_color.cast <double>().replicate(1, numF).transpose();
	
	color_per_face = beta.cwiseProduct(LowDistCol) + alpha.cwiseProduct(HighDistCol);
}