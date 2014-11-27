#ifndef CLASSDEF_GAMEDATA
#define CLASSDEF_GAMEDATA

#include "Common.h"
//{"gamelist":[
//{"cate":"9,0",
//"number":"0",
//"date":"20131104",
//"ver":"0",
//"pad2key":"1",
//"text":"��ׁI�͂邩�ޕ��܂ŁI",
//"title":"XchuBalloon",
//"gver":"0",
//"exe":".\Game\0_xchuballoon\0\XchuBalloon\XchuBalloon.exe",
//"idname":"xchuballoon"}]}

#define GAMEDATA_DIR  0
#define GAMEDATA_JSON 1

struct GameDataList
{
	int number;              // �Q�[���i���o�[�B
	char *idname;            // �ŗL���ʎq�B
	char *title[ 2 ];        // �Q�[���^�C�g���B
	int ver[ 2 ], gver[ 2 ]; // �ݒ�A�Q�[���̃o�[�W�����B
	char *category;          // 1=�f�B���N�g���A2=JSON�B
	char maincategory[ 2 ];  // ���C���J�e�S���B
	char date[ 2 ][ 20 ];    // �X�V���B
	char check;              // �`�F�b�N�̗L���B
	struct GameDataList *next;
};

class GameData
{
private:
	struct GameDataList *gamelist, **gametable;
	int gamemax;

	int categorymax;

	int x, y, w, h;
	int fontsize;
	int mw, mh;
	char format[ 16 ];
	int begin, vgamenum;

	int select;

	virtual struct GameDataList * AnalysisFile( int number, const char *file );
	virtual struct GameDataList * AnalysisJson( const char *json );
	virtual int GetGameMaxFromJSON( int filenum );

	// �v���O���X�o�[�B
	int target, mode, progress, max;
public:
	GameData( void );
	virtual ~GameData( void );

	virtual int Release( void );

	virtual int Init( int categorymax );

	virtual unsigned int GetGameMax( void );
	virtual int IsChecked( unsigned int gamenum );
	virtual int Unckeck( unsigned int gamenum );
	virtual char *GetIdName( unsigned int gamenum );
	virtual int GetGameNumber( unsigned int gamenum );
	virtual int GetGameVersion( unsigned int gamenum );

	virtual int SetTarget( int target = -1 );
	virtual int SetProgressMode( int mode );
	virtual int SetProgressMax( int max );
	virtual int * GetProgressMax( void );
	virtual int * GetProgressNow( void );

	virtual int LoadFromDirectory( const char *gamedir );
	virtual int LoadFromJson( const char *jsonfile );
	virtual int CreateGameTable( void ); // ���b�N���K�v�B

	virtual int SetArea( int x, int y, int w, int h );
	virtual int Draw( int inputlock ); // ���b�N���K�v�B
};

#endif
