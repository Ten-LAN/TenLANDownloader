#ifndef COMMON_HEADER
#define COMMON_HEADER

#include <Mikan.h>
#include "Message.h"

extern class Message msg;

int MouseOver( int x, int y, int w, int h );

// ! スクロールバーの描画。
/* !
\param x 描画X座標。
\param y 描画Y座標。
\param high 高さ。
\param all 全ての件数。
\param view 描画件数。
\param 開始場所。
*/
int DrawScroll( int x, int y, int high, int all, int view, int begin );
#endif
