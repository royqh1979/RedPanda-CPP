#include <raylib.h>
#include <rdrawing.h>
#include <math.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

/**
 * @brief 计算基圆、外旋轮半径和笔尖在外旋轮中位置
 * 
 * @param baseL 基圆相对半径
 * @param outerL 外旋轮相对半径
 * @param pointL 笔尖在外旋轮中相对位置
 * @param pBaseR 基圆实际半径
 * @param pOuterR 外旋轮实际半径
 * @param pPointR 笔尖在滚动圆中实际位置
 */
void updateRadius(float baseL, float outerL,float pointL, float *pBaseR, float *pOuterR, float *pPointR) {
	int totalL=baseL+outerL;
	if (pointL>outerL)
		totalL+=pointL;
	else
		totalL+=outerL;
	int totalR = 340;
	int remainder = totalR % totalL;
	if (remainder!=0) {
		if (remainder < totalL / 2) {
			totalR -= remainder;
		} else {
			totalR += ( totalL - remainder);
		}
	}
	*pBaseR = (totalR) / totalL * baseL;
	*pOuterR = (totalR) / totalL * outerL;
	*pPointR = (totalR) / totalL * pointL;
}


int main() {
	float baseL=2;	//默认基圆相对半径
	float outerL=13;	//默认外旋轮相对半径
	float pointL=3;	//默认笔尖在滚动圆中相对位置
	float baseR,outerR,pointR;
	int cx=350,cy=350;
	float speed = 1;
	
	Color trackColor = BLUE;
	updateRadius(baseL, outerL, pointL, &baseR, &outerR, & pointR);
	
	//初始化绘图窗口
	InitWindow(1000,700,"Epitrochoid");
	SetTraceLogLevel(LOG_WARNING);
	SetTargetFPS(60);
	
	//读取字体文件
	char guichars[]="外旋轮基圆笔尖半径速度颜色清除: 0123456789xABCDEF";
	// 将字符串中的字符逐一转换成Unicode码点，得到码点表
	int codepointsCount;
	int *codepoints=LoadCodepoints(guichars,&codepointsCount);
	// 读取仅含码点表中各字符的字体
	Font font = LoadFontEx("c:\\windows\\fonts\\simhei.ttf",20,codepoints,codepointsCount);
	// 释放码点表
	UnloadCodepoints(codepoints);
	
	
	//设置GUI控件的字体和大小
	GuiSetFont(font);
	GuiSetStyle(DEFAULT,TEXT_SIZE,20);
	
	// 轨迹图层
	Image trackImage=GenImageColor(700,700,WHITE);
	// 绘制图层边框
	ImageFillRectangleEx(&trackImage,0,0,700,700,LIGHTGRAY);
	ImageFillRectangleEx(&trackImage,5,5,690,690,WHITE);
	
	//转动圆图层
	Image circlesImage = GenImageColor(700,700,BLANK);
	float r=0;
	int lastx,lasty;
	lasty=cy;
	lastx=cx+(baseR+outerR-pointR);
	int frameCount = 0;
	
	//主绘图循环
	while(!WindowShouldClose()) { //窗口未被关闭
		//使用raygui绘制控制面板
		float newOuterL = outerL, newBaseL = baseL, newPointL = pointL;
		GuiSliderBar((Rectangle){ 90, 20, 180, 30 },"外旋轮半径",TextFormat("%i", (int)outerL), &newOuterL, 1, 50);
		GuiSliderBar((Rectangle){ 90, 60, 180, 30 },"基圆半径",TextFormat("%i", (int)baseL), &newBaseL, 1, 50);
		GuiSliderBar((Rectangle){ 90, 100, 180, 30 },"笔尖半径",TextFormat("%i", (int)pointL), &newPointL, 1, 50);
		GuiSliderBar((Rectangle){ 90, 150, 180, 30 },"速度",TextFormat("%i", (int)speed), &speed, 1, 50);
		GuiLabel((Rectangle){ 20, 220, 180, 30 },TextFormat("颜色: 0x%02X%02X%02X",(int)(trackColor.r), (int)(trackColor.g),(int)(trackColor.b)));
		GuiColorPicker((Rectangle){ 50, 250, 196, 192 }, NULL, &trackColor);
		int doClear = GuiButton((Rectangle){ 120, 500, 80, 30 },"清除");
		if (newOuterL!=outerL || newBaseL!=baseL || newPointL!=pointL) {
			//圆参数有变化，清除轨迹图层
			if (newOuterL!=outerL) 
				pointL=newOuterL;
			else
				pointL=newPointL;
			outerL=newOuterL;
			baseL=newBaseL;
			updateRadius(baseL, outerL, pointL, &baseR, &outerR, & pointR);
			lasty=cy;
			lastx=cx+(baseR+outerR-pointR);
			r=0;
			ImageClearBackground(&trackImage,WHITE); 
			ImageFillRectangleEx(&trackImage,0,0,700,700,LIGHTGRAY);
			ImageFillRectangleEx(&trackImage,5,5,690,690,WHITE);			
		} else if (doClear) {
			//按下了清除按钮，清除轨迹图层
			ImageClearBackground(&trackImage,WHITE);
			ImageFillRectangleEx(&trackImage,0,0,700,700,LIGHTGRAY);
			ImageFillRectangleEx(&trackImage,5,5,690,690,WHITE);						
		}
		
		//更新圆位置
		r+=0.01;
		float outerCX=cx+ (baseR+outerR)*cos(r);
		float outerCY=cy+ (baseR+outerR)*sin(r);
		float theta = r * (baseL+outerL) / outerL; 
		int x=round(outerCX - pointR * cos(theta));
		int y=round(outerCY - pointR * sin(theta));		
		
		//更新轨迹
		ImageDrawLineEx(&trackImage,lastx,lasty,x,y,3,trackColor);
		
		frameCount++;
		if (frameCount>=speed) {
			//更新转动圆图层中各圆的位置
			ImageClearBackground(&circlesImage,BLANK);
			
			ImageDrawCircleEx(&circlesImage,cx,cy,baseR,1,LIGHTRED);
			ImageDrawCircleEx(&circlesImage,outerCX,outerCY,outerR,1,LIGHTSLATEGRAY);
			ImageDrawLineEx(&circlesImage,cx,cy,outerCX,outerCY,1,LIGHTRED);
			ImageDrawLineEx(&circlesImage,x,y,outerCX,outerCY,1,LIGHTSLATEGRAY);
			ImageDrawPointEx(&circlesImage,x,y,7,RED);
			
			//将图层绘制到屏幕中（GPU绘图）
			Texture trackTexture = LoadTextureFromImage(trackImage);
			Texture circlesTexture = LoadTextureFromImage(circlesImage);
			BeginDrawing();
			ClearBackground(WHITE);
			DrawTexture(trackTexture,300,0,WHITE);
			DrawTexture(circlesTexture,300,0,WHITE);
			EndDrawing();
			UnloadTexture(circlesTexture);
			UnloadTexture(trackTexture);
			frameCount=0;			
		}
		
		lastx=x;
		lasty=y;
	}
	
	//释放图层资源
	UnloadImage(circlesImage);
	UnloadImage(trackImage);
	//关闭窗口
	CloseWindow();
}