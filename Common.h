#ifndef COMMON_HEADER
#define COMMON_HEADER

#include <Mikan.h>
#include "Message.h"

extern class Message msg;

int MouseOver( int x, int y, int w, int h );

// ! �X�N���[���o�[�̕`��B
/* !
\param x �`��X���W�B
\param y �`��Y���W�B
\param high �����B
\param all �S�Ă̌����B
\param view �`�挏���B
\param �J�n�ꏊ�B
*/
int DrawScroll( int x, int y, int high, int all, int view, int begin );
#endif
