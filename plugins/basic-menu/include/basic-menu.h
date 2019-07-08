#ifndef RDS_PLUGINS_BASIC_MENU_H
#define RDS_PLUGINS_BASIC_MENU_H

#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/igl_inline.h>
#include <map>

namespace rds
{
	namespace plugins
	{
		class BasicMenu : public igl::opengl::glfw::imgui::ImGuiMenu
		{
		protected:
			std::map<unsigned int, std::string> data_id_to_name;
			bool test_bool;

		public:
			BasicMenu();
			IGL_INLINE virtual void draw_viewer_menu() override;
			void set_name_mapping(unsigned int data_id, std::string name);
		};
	}
}

#endif
