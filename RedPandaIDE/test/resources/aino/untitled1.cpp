#define AINO_IMPLEMENTATION          // 只能出现在一个编译单元里
#include "aino.hpp"

#include <string>

int main() {
	aino::Window win(aino::WindowConfig{880, 560, "aino 示例"});
	if (!win.ok()) return 1;
	
	std::string email;
	bool notify = true;
	
	win.run([&](aino::Ui& ui) {
		ui.panel({24, 24, 400, 240});
		aino::Stack col = ui.vbox({24, 24, 400, 240}, 10.0f, 18.0f);
		ui.heading("设置", col.next(34.0f), aino::Style().left());
		ui.input(&email, col.next(24.0f), aino::Style().hint("you@example.com"));
		ui.checkbox(&notify, "邮件通知", col.next(22.0f));
		if (ui.button("保存", col.next(24.0f), aino::Style().primary())) {
			win.set_title("设置已保存");
		}
	});
	return 0;
}
