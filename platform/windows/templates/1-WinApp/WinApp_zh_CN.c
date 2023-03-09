#include <windows.h>

/* 在这里处理所有的窗口消息（包括输入） */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		
		/* 销毁（关闭）窗口时，让主线程退出 */
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		
		/* 使用默认的输出过程来处理所有其他消息 */
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

/* Win32 GUI程序的“main”函数：程序从这里开始执行 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc;	/* 用于设置窗口的属性 */
	HWND hwnd; 		/* 我们的窗口句柄（H代表handle（句柄），WND代表windows（窗口）） */
	MSG msg; 		/* 用于临时保存收到的消息 */

	/* 先将整个结构全部置零，然后再设置需要的字段 */
	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc; /* This is where we will send messages to */
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	
	/* COLOR_WINDOWS+1为白色。Ctrl+鼠标左键点击COLOR_WINDOW可以查看它的定义 */
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = "WindowClass";
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* 加载标准图标 */
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* 加载标准图标 */

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"WindowClass","Caption",WS_VISIBLE|WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, /* x */
		CW_USEDEFAULT, /* y */
		640, /* 窗口宽度 */
		480, /* 窗口高度 */
		NULL,NULL,hInstance,NULL);

	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	/*
	  这里是我们程序的核心部分。所有的消息（包括输入）都通过这个循环
	  调用WndProc函数处理。注意GetMessage会程序暂时休眠，直到它收到
	  任意消息为止。所以这个循环不会导致过高的占用CPU。
	*/
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* 如果没有发生错误，且收到了任意消息... */
		TranslateMessage(&msg); /* 将消息中的键盘码转换为对应的字符 */
		DispatchMessage(&msg); /* 调用WndProc函数处理消息 */
	}
	return msg.wParam;
}
