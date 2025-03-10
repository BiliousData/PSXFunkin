/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "boot/character.h"
#include "boot/mem.h"
#include "boot/archive.h"
#include "boot/stage.h"
#include "boot/random.h"
#include "boot/main.h"

//Boyfriend player assets
static u8 char_bf_arc_weeb[] = {
	#include "iso/bf/weeb.arc.h"
};

//Boyfriend skull fragments
static SkullFragment char_bfweeb_skull[15] = {
	{ 1 * 8, -87 * 8, -13, -13},
	{ 9 * 8, -88 * 8,   5, -22},
	{18 * 8, -87 * 8,   9, -22},
	{26 * 8, -85 * 8,  13, -13},
	
	{-3 * 8, -82 * 8, -13, -11},
	{ 8 * 8, -85 * 8,  -9, -15},
	{20 * 8, -82 * 8,   9, -15},
	{30 * 8, -79 * 8,  13, -11},
	
	{-1 * 8, -74 * 8, -13, -5},
	{ 8 * 8, -77 * 8,  -9, -9},
	{19 * 8, -75 * 8,   9, -9},
	{26 * 8, -74 * 8,  13, -5},
	
	{ 5 * 8, -73 * 8, -5, -3},
	{14 * 8, -76 * 8,  9, -6},
	{26 * 8, -67 * 8, 15, -3},
};

//Boyfriend Weeb player types
enum
{
	BFWeeb_ArcMain_Weeb0,
	BFWeeb_ArcMain_Weeb1,
	
	BFWeeb_ArcMain_Max,
};

enum
{
	BFWeeb_ArcDead_DeadW0,
	
	BFWeeb_ArcDead_Max,
};

#define BFWeeb_Arc_Max BFWeeb_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main, arc_dead;
	CdlFILE file_dead_arc; //dead.arc file position
	IO_Data arc_ptr[BFWeeb_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
	
	u8 retry_bump;
	
	SkullFragment skull[COUNT_OF(char_bfweeb_skull)];
	u8 skull_scale;
} Char_BF;

//Boyfriend Weeb player definitions
static const CharFrame char_bfweeb_frame[] = {
	{BFWeeb_ArcMain_Weeb0, {  0,   0,  60,  55}, { 35,  52}}, //0 idle 1
	{BFWeeb_ArcMain_Weeb0, { 61,   0,  61,  56}, { 35,  53}}, //1 idle 2
	{BFWeeb_ArcMain_Weeb0, {123,   0,  59,  58}, { 33,  55}}, //2 idle 3
	{BFWeeb_ArcMain_Weeb0, {183,   0,  58,  59}, { 31,  56}}, //3 idle 4
	{BFWeeb_ArcMain_Weeb0, {  0,  56,  58,  58}, { 32,  55}}, //4 idle 5
	
	{BFWeeb_ArcMain_Weeb0, { 59,  57,  54,  57}, { 34,  53}}, //5 left 1
	{BFWeeb_ArcMain_Weeb0, {114,  59,  55,  57}, { 33,  53}}, //6 left 2
	
	{BFWeeb_ArcMain_Weeb0, {170,  60,  55,  52}, { 31,  48}}, //7 down 1
	{BFWeeb_ArcMain_Weeb0, {  0, 115,  54,  53}, { 31,  49}}, //8 down 2
	
	{BFWeeb_ArcMain_Weeb0, { 55, 116,  57,  64}, { 26,  60}}, //9 up 1
	{BFWeeb_ArcMain_Weeb0, {113, 117,  58,  63}, { 27,  59}}, //10 up 2
	
	{BFWeeb_ArcMain_Weeb0, {172, 113,  57,  56}, { 22,  52}}, //11 right 1
	{BFWeeb_ArcMain_Weeb0, {  0, 169,  55,  56}, { 22,  52}}, //12 right 2
	
	{BFWeeb_ArcMain_Weeb0, { 56, 181,  54,  57}, { 33,  53}}, //13 left miss 1
	{BFWeeb_ArcMain_Weeb0, {113, 181,  55,  57}, { 33,  53}}, //14 left miss 2
	
	{BFWeeb_ArcMain_Weeb0, {169, 170,  53,  56}, { 31,  52}}, //15 down miss 1
	{BFWeeb_ArcMain_Weeb1, {  0,   0,  54,  53}, { 31,  49}}, //16 down miss 2
	
	{BFWeeb_ArcMain_Weeb1, { 55,   0,  60,  61}, { 28,  57}}, //17 up miss 1
	{BFWeeb_ArcMain_Weeb1, {116,   0,  58,  63}, { 27,  59}}, //18 up miss 2
	
	{BFWeeb_ArcMain_Weeb1, {175,   0,  54,  56}, { 22,  52}}, //19 right miss 1
	{BFWeeb_ArcMain_Weeb1, {  0,  54,  55,  56}, { 22,  52}}, //20 right miss 2
};

static const Animation char_bfweeb_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3, 4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5,  6, ASCR_BACK, 1}},            //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 1}},            //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 1}},            //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_UpAlt
	{2, (const u8[]){11, 12, ASCR_BACK, 1}},            //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_RightAlt
	
	{2, (const u8[]){13, 14, ASCR_BACK, 1}},            //CharAnim_Left
	{2, (const u8[]){15, 16, ASCR_BACK, 1}},            //CharAnim_Down
	{2, (const u8[]){17, 18, ASCR_BACK, 1}},            //CharAnim_Up
	{2, (const u8[]){19, 20, ASCR_BACK, 1}},            //CharAnim_Right
	
	{2, (const u8[]){20, 21, 22, ASCR_BACK, 1}},        //PlayerAnim_Peace
	{2, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //PlayerAnim_Sweat
};

//Boyfriend Weeb player functions
void Char_BFWeeb_SetFrame(void *user, u8 frame)
{
	Char_BF *this = (Char_BF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bfweeb_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BFWeeb_Tick(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	//Handle animation updates
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0 ||
	    (character->animatable.anim != CharAnim_Left &&
	     character->animatable.anim != CharAnim_LeftAlt &&
	     character->animatable.anim != CharAnim_Down &&
	     character->animatable.anim != CharAnim_DownAlt &&
	     character->animatable.anim != CharAnim_Up &&
	     character->animatable.anim != CharAnim_UpAlt &&
	     character->animatable.anim != CharAnim_Right &&
	     character->animatable.anim != CharAnim_RightAlt))
		Character_CheckEndSing(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		//Perform idle dance
		if (Animatable_Ended(&character->animatable) &&
			(character->animatable.anim != CharAnim_Left &&
		     character->animatable.anim != CharAnim_LeftAlt &&
		     character->animatable.anim != PlayerAnim_LeftMiss &&
		     character->animatable.anim != CharAnim_Down &&
		     character->animatable.anim != CharAnim_DownAlt &&
		     character->animatable.anim != PlayerAnim_DownMiss &&
		     character->animatable.anim != CharAnim_Up &&
		     character->animatable.anim != CharAnim_UpAlt &&
		     character->animatable.anim != PlayerAnim_UpMiss &&
		     character->animatable.anim != CharAnim_Right &&
		     character->animatable.anim != CharAnim_RightAlt &&
		     character->animatable.anim != PlayerAnim_RightMiss) &&
			(stage.song_step & 0x7) == 0)
			character->set_anim(character, CharAnim_Idle);
	}
	
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BFWeeb_SetFrame);
	Character_Draw(character, &this->tex, &char_bfweeb_frame[this->frame]);
}

void Char_BFWeeb_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_BFWeeb_Free(Character *character)
{
	(void)character;
}

Character *Char_BFWeeb_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BF *this = Mem_Alloc(sizeof(Char_BF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BFWeeb_New] Failed to allocate boyfriend weeb object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BFWeeb_Tick;
	this->character.set_anim = Char_BFWeeb_SetAnim;
	this->character.free = Char_BFWeeb_Free;
	
	Animatable_Init(&this->character.animatable, char_bfweeb_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_i = 7;
	
	this->character.focus_x = FIXED_DEC(-34,1);
	this->character.focus_y = FIXED_DEC(-40,1);
	this->character.focus_zoom = FIXED_DEC(2,1);
	
	//Load art
	const char **pathp = (const char *[]){
		"weeb0.tim",  //BFWeeb_ArcMain_Weeb0
		"weeb1.tim",  //BFWeeb_ArcMain_Weeb1
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_bf_arc_weeb, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	//Initialize player state
	this->retry_bump = 0;
	
	//Copy skull fragments
	memcpy(this->skull, char_bfweeb_skull, sizeof(char_bfweeb_skull));
	this->skull_scale = 64;
	
	SkullFragment *frag = this->skull;
	for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BF, skull); i++, frag++)
	{
		//Randomize trajectory
		frag->xsp += RandomRange(-4, 4);
		frag->ysp += RandomRange(-2, 2);
	}
	
	return (Character*)this;
}
