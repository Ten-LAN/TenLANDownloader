#include "Common.h"
#include "Config.h"
#include "GameData.h"

class Config conf;
class Message msg;
class GameData game;

extern "C" __declspec( dllexport ) int ZipNums( const char *ZipFilename );
extern "C" __declspec( dllexport ) int Unzip( const char *ZipFilename, const char *TargetPath, int *count );

int( *FileNumsInZip )( const char * );
int( *UnzipFile )( const char *, const char *, int * );

const char ZIPFILE[] = "tmp.zip";

HINSTANCE hdll;

#define PATHSIZE 260

enum
{
	MODE_DEFAULT,
	MODE_CONFIG,
};

struct WINDOWHEADER
{
	int timer;
	int wx, wy, drg;

	// �A�C�R���̕\���̑傫���ێ��̂��߂̒l�B
	int close;
	int config;
	int game;
	int jsondownload;
	int download;

	// �v���O���X�o�[�̐���B
	int progress, progressmax;
	// �Q�[���p�̃v���O���X�o�[�̐���B
	struct
	{
		unsigned int gamenum;
		int now, max;
		char *basead;
	} gameprogress;

	// ��ʃT�C�Y�B
	int width, height;

	// ���݉����̏��������ǂ����B
	int working;

	// �E�B���h�E���[�h�B
	int mode;

	// �J�����g�f�B���N�g���B
	char curdir[ PATHSIZE ];
} window = {};

int MouseOver( int x, int y, int w, int h )
{
	int mx, my;
	mx = MikanInput->GetMousePosX();
	my = MikanInput->GetMousePosY();
	return ( x < mx && mx < x + w && y < my && my < y + h );
}

int DrawScroll( int x, int y, int high, int all, int view, int begin )
{
	//int rbegin = begin;
	int length = 0;
	// �X�N���[���o�[�B

	// ��B
	MikanDraw->DrawTexture( 0, x, y, 680, 40, 10, 10 );
	MikanDraw->DrawTexture( 0, x, y + 10, 680, 40, 10, 10, DRAW_UD );

	// ���B
	MikanDraw->DrawTexture( 0, x, y + high - 20, 680, 40, 10, 10, DRAW_LR );
	MikanDraw->DrawTexture( 0, x, y + high - 10, 680, 40, 10, 10, DRAW_LR | DRAW_UD );

	// �����B
	if(all <= view)
	{
		// �S�����\�������ȉ��̏ꍇ�B
		length = high - 40;
	} else
	{
		length = ( high - 40 ) * view / all;
	}

	begin = ( high - 40 ) * begin / all;

	// �o�[�{�́B
	MikanDraw->DrawTexture( 0, x, y + 10 + begin, 680, 50, 10, 10 ); // ��B
	MikanDraw->DrawTextureScaling( 0, x, y + 20 + begin, 680, 60, 10, 10, 10, length, 0 ); // ���i�B
	MikanDraw->DrawTexture( 0, x, y + 20 + begin + length, 680, 70, 10, 10 ); // ���B

	if(MikanInput->GetMouseNum( 0 ) == 1)
	{
		if(MouseOver( x, y, 10, 20 ))
		{
			return -1;
		} else if(MouseOver( x, y + high - 20, 10, 20 ))
		{
			return 1;
		}
	}

	//return rbegin;
	return 0;
}

// JSON�̃A�b�v�f�[�g�B
int JsonUpdate_( int *progress, int *progressmax )
{
	char *url;
	unsigned int size;

	if(conf.GetBaseURL() == NULL)
	{
		msg.SetMessage( "[FAIL] Failure json update." );
		return -1;
	}

	msg.SetMessage( "Begin download Gamelist." );

	size = strlen( conf.GetBaseURL() ) + 14;
	url = (char *)calloc( size, sizeof( char ) );
	sprintf_s( url, size, "%s/gamelist.cgi", conf.GetBaseURL() );
	MikanNet->HttpGet( url, "gamelist.json", progressmax, progress );
	free( url );

	msg.SetMessage( "End download." );

	msg.SetMessage( "Begin Analysis gamelist.json." );
	game.LoadFromJson( "gamelist.json" );
	msg.SetMessage( "End Analysis." );
	return 0;
}

int JsonUpdate( void )
{
	MikanSystem->Lock( 0 );
	++window.working;
	JsonUpdate_( &( window.progress ), &( window.progressmax ) );
	--window.working;
	MikanSystem->Unlock( 0 );
	return 0;
}

// �Q�[���̃_�E�����[�h�ƃC���X�g�[���B
int GameDownload_( void )
{
	char *adzip = NULL;
	unsigned int size;
	unsigned int gamenum = window.gameprogress.gamenum;

	msg.SetMessage( "Begin Download..." );
	// �T�C�Y�擾�Ƃ����ă|�C���^�˂����ށB
	if(game.GetIdName( gamenum ))
	{
		// �x�[�X�A�h���X/Game/���l_ID/
		size = strlen( conf.GetBaseURL() ) + 6 + 7 + 1 + strlen( game.GetIdName( gamenum ) ) + 1 + 1;
	} else
	{
		// �x�[�X�A�h���X/Game/���l/
		size = strlen( conf.GetBaseURL() ) + 6 + 7 + 1 + 1;
	}
	window.gameprogress.basead = (char *)calloc( size, sizeof( char ) );
	if(game.GetIdName( gamenum ))
	{
		sprintf_s( window.gameprogress.basead, size, "%s/Game/%d_%s/", conf.GetBaseURL(), game.GetGameNumber( gamenum ), game.GetIdName( gamenum ) );
	} else
	{
		sprintf_s( window.gameprogress.basead, size, "%s/Game/%d/", conf.GetBaseURL(), game.GetGameNumber( gamenum ) );
	}

	// ZIP�A�h���X�̍쐬�B
	// �x�[�X�A�h���X�o�[�W����.zip
	size = strlen( window.gameprogress.basead ) + 7 + 4 + 1;
	adzip = (char *)calloc( size, sizeof( char ) );
	sprintf_s( adzip, size, "%s%d.zip", window.gameprogress.basead, game.GetGameVersion( gamenum ) );

	msg.SetMessageFormat( "Address: %s", adzip );
	// �_�E�����[�h�B
	MikanNet->HttpGet( adzip, ZIPFILE, game.GetProgressMax(), game.GetProgressNow() );//&(window.gameprogress.max), &(window.gameprogress.now) );

	free( adzip );

	return 0;
}

int GameInstall_( void )
{
	char *ad_base = NULL, *adconf = NULL, *adss = NULL, *dir = NULL;
	unsigned int size;
	unsigned int gamenum = window.gameprogress.gamenum;

	// �T�C�Y�擾�Ƃ����ă|�C���^�˂����ށB
	// �C���X�g�[��(ZIP�𓀂��邾��)�B
	game.SetProgressMax( FileNumsInZip( ZIPFILE ) );
	UnzipFile( ZIPFILE, ".", game.GetProgressMax() );//ZipNums( ZIPFILE );

	// �J�����g�f�B���N�g���ړ�
	if(game.GetIdName( gamenum ))
	{
		// Game/���l_idname/
		size = strlen( "./Game/" ) + 7 + 1 + strlen( game.GetIdName( gamenum ) ) + 1 + 1;
	} else
	{
		// Game/���l_idname/
		size = strlen( "./Game/" ) + 7 + 1 + 1;
	}
	dir = (char *)calloc( size, sizeof( char ) );
	if(game.GetIdName( gamenum ))
	{
		sprintf_s( dir, size, "./Game/%d_%s/", game.GetGameNumber( gamenum ), game.GetIdName( gamenum ) );
	} else
	{
		sprintf_s( dir, size, "./Game/%d/", game.GetGameNumber( gamenum ) );
	}
	SetCurrentDirectory( dir );

	game.SetProgressMode( 0 );

	// �ݒ�A�h���X�̍쐬�B
	// �x�[�X�A�h���Xdata.ini
	size = strlen( window.gameprogress.basead ) + 8 + 1;
	adconf = (char *)calloc( size, sizeof( char ) );
	sprintf_s( adconf, size, "%sdata.ini", window.gameprogress.basead );
	MikanNet->HttpGet( adconf, "data.ini", game.GetProgressMax(), game.GetProgressNow() );

	// �X�N���[���V���b�g�A�h���X�̍쐬�B
	// �x�[�X�A�h���Xss.png
	size = strlen( window.gameprogress.basead ) + 6 + 1;
	adss = (char *)calloc( size, sizeof( char ) );
	sprintf_s( adss, size, "%sss.png", window.gameprogress.basead );
	MikanNet->HttpGet( adss, "ss.png", game.GetProgressMax(), game.GetProgressNow() );

	SetCurrentDirectory( window.curdir );

	free( dir );
	free( window.gameprogress.basead );
	free( adconf );
	free( adss );

	return 0;
}

int GameUpdate( void )
{
	int gamemax;
	unsigned int i;

	if(conf.GetBaseURL() == NULL)
	{
		msg.SetMessage( "[FAIL] Failure game update." );
		return -1;
	}

	MikanSystem->Lock( 0 );
	++window.working;

	// �Q�[���ő�l�擾�B
	gamemax = 0;
	for(i = 0 ; i < game.GetGameMax() ; ++i)
	{
		if(game.IsChecked( i ))
		{
			++gamemax;
		}
	}

	for(i = 0 ; i < game.GetGameMax() ; ++i)
	{
		if(game.IsChecked( i ))
		{
			// �������B
			window.gameprogress.gamenum = i;
			window.gameprogress.now = 0;
			window.gameprogress.max = 0;
			window.gameprogress.basead = NULL;
			game.SetTarget( i );
			// �Q�[���_�E�����[�h�B
			game.SetProgressMode( 0 );
			GameDownload_();
			// �Q�[���C���X�g�[���B
			game.SetProgressMode( 1 );
			GameInstall_();
			// �`�F�b�N���O���B
			game.Unckeck( i );
		}
	}
	game.SetTarget();

	// �ēx�����X�V�B
	game.LoadFromDirectory( conf.GetGameDirectory() );

	--window.working;
	MikanSystem->Unlock( 0 );

	return 0;
}

// �X���b�h�Ŏ��s�B
int FirstLoad( void )
{
	MikanSystem->Lock( 0 );
	++window.working;

	window.progressmax = 100;
	window.progress = 0;
	// �ݒ�t�@�C����ǂށB
	conf.Load( "./config.ini" );
	window.progress = 10;

	// �ő�J�e�S�����̓o�^�B
	game.Init( conf.GetCategoryMax() );

	// �f�B���N�g�����̃Q�[�������擾�B
	game.LoadFromDirectory( conf.GetGameDirectory() );
	window.progress = 40;

	// JSON�_�E�����[�h�B
	JsonUpdate_( NULL, NULL );

	window.progress = 70;

	// JSON��́B
	msg.SetMessage( "Begin analysis gamelist.json" );

	window.progress = 100;

	msg.SetMessage( "FirstLoad() end." );

	--window.working;
	MikanSystem->Unlock( 0 );
	return 0;
}

// �E�B���h�E�̈ړ��B
int MoveWindow( void )
{
	int my;
	my = MikanInput->GetMousePosY();
	if(MikanInput->GetMouseNum( 0 ) == 1 && my < 40)
	{
		window.wx = MikanInput->GetMousePosX();
		window.wy = MikanInput->GetMousePosY();
		window.drg = 1;
	} else if(window.drg && MikanInput->GetMouseNum( 0 ) == -1)
	{
		window.drg = 0;
	} else if(window.drg)
	{
		window.wx = MikanWindow->GetPositionX() - ( window.wx - MikanInput->GetMousePosX() );
		window.wy = MikanWindow->GetPositionY() - ( window.wy - MikanInput->GetMousePosY() );
		MikanWindow->SetPositionXY( window.wx, window.wy );
		window.wx = MikanInput->GetMousePosX();
		window.wy = MikanInput->GetMousePosY();
	}
	return 0;
}

// �E�B���h�E�̏�B
int DrawHeader( void )
{
	//int mx, my;
	int csize;
	int x_close, x_config, x_json, x_game, x_download;
	const char *msg = NULL;

	//mx = MikanInput->GetMousePosX();
	//my = MikanInput->GetMousePosY();

	x_close = 640 - ( 20 / 2 + 10 );
	x_json = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 );
	x_download = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 ) * 2;

	x_config = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 ) * 4;
	x_game = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 ) * 5;

	if(MikanInput->GetMousePosY() < 40)
	{
		// �I���{�^���B
		if(MouseOver( x_close - 15, 5, 30, 30 )) //x_close - 15 < mx && mx < x_close + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.close < 10)
			{
				++window.close;
			}
			msg = "Exit";
			if(MikanInput->GetMouseNum( 0 ) == -1)
			{
				return 1;
			}
		} else if(window.close)
		{
			--window.close;
		}

		// �ݒ�{�^���B
		if(MouseOver( x_config - 15, 5, 30, 30 )) //x_config - 15 < mx && mx < x_config + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.config < 10)
			{
				++window.config;
			}
			msg = "Config";
			if(MikanInput->GetMouseNum( 0 ) == -1)
			{
				window.mode = MODE_CONFIG;
			}
		} else if(window.config)
		{
			--window.config;
		}

		// JSON�A�b�v�f�[�g�{�^���B
		if(MouseOver( x_json - 15, 5, 30, 30 )) //x_json - 15 < mx && mx < x_json + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.jsondownload < 10)
			{
				++window.jsondownload;
			}
			msg = "Database update";
			if(MikanInput->GetMouseNum( 0 ) == -1 && MikanSystem->GetThreadHandle( 0 ) == NULL)
			{
				MikanSystem->RunThread( 0, JsonUpdate );
			}
		} else if(window.jsondownload)
		{
			--window.jsondownload;
		}

		// JSON�A�b�v�f�[�g�{�^���B
		if(MouseOver( x_download - 15, 5, 30, 30 )) //x_json - 15 < mx && mx < x_json + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.download < 10)
			{
				++window.download;
			}
			msg = "Download & Install";
			if(MikanInput->GetMouseNum( 0 ) == -1 && MikanSystem->GetThreadHandle( 0 ) == NULL)
			{
				MikanSystem->RunThread( 0, GameUpdate );
			}
		} else if(window.download)
		{
			--window.download;
		}

		// �Q�[���ꗗ�{�^���B
		if(MouseOver( x_game - 15, 5, 30, 30 )) //x_json - 15 < mx && mx < x_json + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.game < 10)
			{
				++window.game;
			}
			msg = "Game List";
			if(MikanInput->GetMouseNum( 0 ) == -1)
			{
				window.mode = MODE_DEFAULT;
			}
		} else if(window.game)
		{
			--window.game;
		}
	} else
	{
		if(0 < window.close)
		{
			--window.close;
		}
		if(0 < window.config)
		{
			--window.config;
		}
		if(0 < window.jsondownload)
		{
			--window.jsondownload;
		}//window.mode != MODE_CONFIG
		if(0 < window.download)
		{
			--window.download;
		}
		if(0 < window.game)
		{
			--window.game;
		}
	}

	// �v���O���X�o�[�B
	if(window.progress > window.progressmax)
	{
		window.progress = window.progressmax;
	}
	if(window.progressmax > 0)
	{
		MikanDraw->DrawTexture( 0, 0, 30, 640 + 50, 40, 105 * window.progress / window.progressmax, 10 );
		MikanDraw->SetAlpha( 0, 255 * abs( window.timer % 121 - 60 ) / 60 );
		MikanDraw->DrawTexture( 0, 0, 30, 640 + 50, 50, 105 * window.progress / window.progressmax, 10 );
		MikanDraw->SetColor( 0, 0xFFFFFFFF );
	}
	if(window.progress >= window.progressmax)
	{
		window.progress = window.progressmax = 0;
	}

	// ����B
	csize = 30 + (int)( 10.0 * window.close / 10.0 );
	MikanDraw->DrawTextureScalingC( 0, x_close, 15 + 5, 640, 0, 40, 40, csize, csize, 0 );

	// �R���t�B�O�B
	if(window.mode == MODE_CONFIG)
	{
		MikanDraw->DrawTexture( 0, x_config - 20, 0, 840, 40, 40, 40 );
		MikanDraw->DrawTextureC( 0, x_config, 15 + 5, 640 + 80, 0, 40, 40 );
		window.config = 10;
	} else
	{
		csize = 30 + (int)( 10.0 * window.config / 10.0 );
		MikanDraw->DrawTextureScalingC( 0, x_config, 15 + 5, 640 + 80, 0, 40, 40, csize, csize, 0 );
	}

	// `�Q�[���ꗗ�B�B
	if(window.mode == MODE_DEFAULT)
	{
		MikanDraw->DrawTexture( 0, x_game - 20, 0, 840, 40, 40, 40 );
		MikanDraw->DrawTextureC( 0, x_game, 15 + 5, 640 + 160, 0, 40, 40 );
		window.game = 10;
	} else
	{
		csize = 30 + (int)( 10.0 * window.game / 10.0 );
		MikanDraw->DrawTextureScalingC( 0, x_game, 15 + 5, 640 + 160, 0, 40, 40, csize, csize, 0 );
	}

	// JSON�_�E�����[�h�B
	csize = 30 + (int)( 10.0 * window.jsondownload / 10.0 );
	MikanDraw->DrawTextureScalingC( 0, x_json, 15 + 5, 640 + 40, 0, 40, 40, csize, csize, 0 );

	// �Q�[���_�E�����[�h�B
	csize = 30 + (int)( 10.0 * window.download / 10.0 );
	MikanDraw->DrawTextureScalingC( 0, x_download, 15 + 5, 640 + 120, 0, 40, 40, csize, csize, 0 );

	// ���j���[�I�����̐����B
	if(msg)
	{
		MikanDraw->Printf( 1, 110, 30, msg );
	}
	return 0;
}

HINSTANCE LoadUnzipDLL( const char *dll )
{
	HINSTANCE hdll;
	if(( hdll = LoadLibrary( dll ) ) == NULL)
	{
		return NULL;
	}
	FileNumsInZip = ( int( *)( const char * ) )GetProcAddress( hdll, "ZipNums" );
	UnzipFile = ( int( *)( const char *, const char *, int * ) )GetProcAddress( hdll, "Unzip" );
	return hdll;
}

void SystemInit( void )
{
	hdll = LoadUnzipDLL( "Unzip.dll" );

	_MikanWindow->SetWindowSize( 640, 480 );
	_MikanWindow->SetWindow( WT_NOFRAME );

	MikanWindow->SetWindowName( "TenLAN Game Downloader" );
	MikanWindow->SetWindowIcon( "GAME_ICON" );

	GetCurrentDirectory( PATHSIZE, window.curdir );

	unsigned long vol;
	// test sound
	unsigned int devmax = mixerGetNumDevs();
	unsigned int i;
	MIXERCAPS mixercaps;
	HMIXER hmixer;
	MIXERLINE mixerline = {};
	MIXERLINECONTROLS controls = {};
	MIXERCONTROL control = {};
	MIXERCONTROLDETAILS details = {};
	MIXERCONTROLDETAILS_UNSIGNED *detailsus;
	MMRESULT mres;
	for(i = 0 ; i < devmax ; ++i)
	{
		mixerGetDevCaps( i, &mixercaps, sizeof( MIXERCAPS ) );
		mixercaps.szPname;// �f�o�C�X�̖��O�B
		mixercaps.cDestinations;// ���C���̐��B
	}
	if(0 < devmax && ( mres = mixerOpen( &hmixer, 0, 0, 0, MIXER_OBJECTF_MIXER ) ) == MMSYSERR_NOERROR)
	{
		mixerline.cbStruct = sizeof( MIXERLINE );
		//mixerline.dwDestination = 0;
		mixerline.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
		if(( mres = mixerGetLineInfo( (HMIXEROBJ)( hmixer ), &mixerline, MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			MIXERR_INVALLINE;
			MMSYSERR_BADDEVICEID;
			MMSYSERR_INVALFLAG;
			if(mres == MMSYSERR_INVALHANDLE)
			{
				int b = 0;
			}
			MMSYSERR_INVALPARAM;
			MMSYSERR_NODRIVER;
			int a = 0;
		}

		controls.cbStruct = sizeof( MIXERLINECONTROLS );
		controls.dwLineID = mixerline.dwLineID;
		controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
		controls.cControls = 1;
		controls.cbmxctrl = sizeof( MIXERCONTROL );
		controls.pamxctrl = &control;
		if(( mres = mixerGetLineControls( (HMIXEROBJ)( hmixer ), &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			mres = 0;
		}


		details.cbStruct = sizeof( MIXERCONTROLDETAILS );
		details.cbDetails = sizeof( MIXERCONTROLDETAILS_UNSIGNED );
		//if ( control.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE )
		//{
		details.cMultipleItems = 0;//control.cMultipleItems;
		//}
		//if ( control.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM )
		//{
		details.cChannels = 1;
		//} else
		//{
		//  details.cChannels = mixerline.cChannels;
		//}
		details.dwControlID = control.dwControlID;
		details.paDetails = (MIXERCONTROLDETAILS_UNSIGNED *)calloc( details.cChannels, sizeof( MIXERCONTROLDETAILS_UNSIGNED ) );

		if(( mres = mixerGetControlDetails( (HMIXEROBJ)( hmixer ), &details, MIXER_GETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			mres = 0;
		}

		control.Bounds.dwMinimum;
		control.Bounds.dwMaximum;

		unsigned int a = ( (MIXERCONTROLDETAILS_UNSIGNED *)details.paDetails )->dwValue;
		( (MIXERCONTROLDETAILS_UNSIGNED *)details.paDetails )->dwValue = control.Bounds.dwMaximum / 3;
		if(( mres = mixerSetControlDetails( (HMIXEROBJ)( hmixer ), &details, MIXER_SETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			mres = 0;
		}
		free( details.paDetails );
		mixerClose( hmixer );
	}
	//vol = auxGetNumDevs();
	//auxGetVolume( 0, &vol );
	//auxSetVolume( 0, 0xFFFF );
}

void UserInit( void )
{
	window.width = 640;//MikanDraw->GetScreenWidth();
	window.height = 480;//MikanDraw->GetScreenHeight();

	// �V�X�e���t�H���g�̐ݒ�B
	MikanDraw->LoadFontFile( NULL, "SYSFONT" );
	MikanDraw->CreateFont( 0, "MigMix 1M", 40, 0xFFFFFFFF );
	MikanDraw->CreateFont( 2, "MigMix 1M", 20, 0xFFFFFFFF );

	// �h�b�g�t�H���g�̐ݒ�B
	MikanDraw->LoadFontFile( NULL, "DOTFONT" );
	MikanDraw->CreateFont( 1, "����S�V�b�N", 8, 0xFFFFFFFF );

	// �V�X�e���e�N�X�`���̏����B
	MikanDraw->CreateTexture( 0, NULL, "SYSIMG", TRC_NONE );

	// �e�X�g�p�V�X�e���T�E���h�̏����B
	MikanSound->Load( 0, NULL, "SYSSOUND" );

	//MikanSound->Play( 0, true );

	// ���b�Z�[�W�����N���X�̏������B
	msg.Init();
	// �\���G���A�̐ݒ�B
	msg.SetArea( 0, 380, 640, 100 );

	game.SetArea( 0, 40, 340, 340 );

	MikanSystem->CreateLock( 0 );
	// �ʃX���b�h�œǂݍ��݁B
	// ���b�Z�[�W�̑���ƕ`��͓���������Ă�̂ő��v�B
	MikanSystem->RunThread( 0, FirstLoad );

	//MikanSystem->SetInactiveWindow( 0 );
}

int MainLoop( void )
{
	int ret;

	//��ʂ̏�����
	MikanDraw->ClearScreen();

	// �w�i�̕`��B
	MikanDraw->DrawTexture( 0, 0, 0, 0, 0, 640, 480 );

	switch(window.mode)
	{
	case MODE_CONFIG: // �R���t�B�O�B
		conf.Draw();
		break;
	default: // �Q�[�����X�g�`��B
		game.Draw( ( window.working == 0 ) );
		break;
	}

	// ���b�Z�[�W�`��B
	msg.Draw();

	// �E�B���h�E�ړ��B
	MoveWindow();

	// �E�B���h�E�̏�B
	ret = DrawHeader();

	// �ҋ@���B
	if(window.working)
	{
		MikanDraw->DrawTextureRotationC( 0, window.width / 2, window.height / 2, 640, 80, 100, 100, MIKAN_PI * ( window.timer % 100 ) / 100.0 );
	}
	++window.timer;
	return ( MikanInput->GetKeyNum( K_ESC ) == 1 ) || ret;
}

void CleanUp( void )
{
	FreeLibrary( hdll );
	msg.Term();
}
