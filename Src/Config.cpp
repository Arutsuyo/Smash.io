#include "Config.h"

#include <fstream>
#include <iostream>

#define _GFX_BACKEND "OGL"
#define _DUAL_CORE_DEFAULT 1
#define _GFX_DEFAULT false
#define _FULLSCREEN_DEFAULT false
#define FILENM "CFG"

#pragma region UtilStrings
std::string PlayerKeyboard =
"Device = Xlib/0/Keyboard Mouse\n"
"Buttons/A = V\n"
"Buttons/B = C\n"
"Buttons/X = X\n"
"Buttons/Y = `Button Y`\n"
"Buttons/Z = Z\n"
"Buttons/Start = Return\n"
"Main Stick/Up = Up\n"
"Main Stick/Down = Down\n"
"Main Stick/Left = Left\n"
"Main Stick/Right = Right\n"
"C-Stick/Up = `Axis C Y -`\n"
"C-Stick/Down = `Axis C Y +`\n"
"C-Stick/Left = `Axis C X -`\n"
"C-Stick/Right = `Axis C X +`\n"
"Triggers/L = space\n"
"D-Pad/Up = `Button D_UP`\n"
"D-Pad/Down = `Button D_DOWN`\n"
"D-Pad/Left = `Button D_LEFT`\n"
"D-Pad/Right = `Button D_RIGHT`\n";

std::string AIController =
"Buttons/A = `Button A`\n"
"Buttons/B = `Button B`\n"
"Buttons/X = `Button X`\n"
"Buttons/Y = `Button Y`\n"
"Buttons/Z = `Button Z`\n"
"Buttons/Start = `Button START`\n"
"D-Pad/Up = `Button D_UP`\n"
"D-Pad/Down = `Button D_DOWN`\n"
"D-Pad/Left = `Button D_LEFT`\n"
"D-Pad/Right = `Button D_RIGHT`\n"
"Triggers/L = `Button L`\n"
"Triggers/R = `Button R`\n"
"Main Stick/Up = `Axis MAIN Y +`\n"
"Main Stick/Down = `Axis MAIN Y -`\n"
"Main Stick/Left = `Axis MAIN X -`\n"
"Main Stick/Right = `Axis MAIN X +`\n"
"C-Stick/Up = `Axis C Y - `\n"
"C-Stick/Down = `Axis C Y + `\n"
"C-Stick/Left = `Axis C X - `\n"
"C-Stick/Right = `Axis C X + `\n";

// These values are pulled from the community provided memory locations 
// spreadsheet available here: 
// https://docs.google.com/spreadsheets/d/1JX2w-r2fuvWuNgGb6D3Cs4wHQKLFegZe2jhbBuIhCG8/edit#gid=5
// The way dolphin parses the locations can be found here:
// https://github.com/dolphin-emu/dolphin/blob/123bbbca2c3382165cf58288e4d65d0974a9c0db/Source/Core/Core/MemoryWatcher.cpp#L48
// And an example of how dolphin composes it's memory can be found here: 
// root/Test Files/mem.cpp


std::string memlocation =
/* 0x80453080 - - - P1 Block - - -	0x00 of P1's static Player Block data. */
// Each player is 0xE90 apart (goes up to Player 6).
"453080 40\n" // Facing Direction	Float.
"453080 60\n" // Stamina HP Lost / Percentage - Currently Displayed 	Short
"453080 68\n" // Falls	Int
"453080 8E\n" // Stocks	Byte
// (Add pointers to this v to index into player)
/* 0x80453080 0xB0	Pointer to Player Entity (Probably don't need)*/
"453080 D1C\n" // Total Damage Received	Float.
"453080 D28\n" // Total Damage Given	Float.

/* 0x80453130 - - - P1 character entity data pointer - - -	Refer to the Char Offset table. Each subsequent player's pointer is stored at +0xE90*/
"453130 10\n" // Action State	The value at this address indicates the action state of the character.Modifying this alone does nothing.
"453130 2C\n" // Facing direction	Float. 1 is right, -1 is left
"453130 80\n" // Horizontal velocity(self - induced)	Float.Changing this will only have an effect if the character is in the air.
"453130 84\n" // Vertical Velocity(self - induced)	Float.Positive = up, negative = down.Changes when the player jumps / falls of his own accord.Does not kill upward.
"453130 8C\n" // Horizontal velocity(attack - induced)	Float.Positive = right, negative = left.
"453130 90\n" // Vertical velocity(attack - induced)	Float.Positive = up, negative = down.Only works when player is in the air.Changes when the player is attacked.Kills upward.
"453130 B0\n" // X(Horizontal) position	Float.Positive is right, negative is left. 00000000 is center.
"453130 B4\n" // Y(Vertical) position	Float.Only works when player is in the air.
"453130 C8\n" // X Delta(Curr - Prev Pos)	Float
"453130 CC\n" // Y Delta(Curr - Prev Pos)	Float
"453130 E0\n" // Ground / Air state	0x00000000 on ground,0x00000001 in air.Changing does nothing.
"453130 894\n" // Action State Frame Counter	Float, resets to 1 with each new action state.
"453130 1968\n" // Number of jumps used(8 - bit)	Gives more jumps if lowered.
"453130 198C\n" // Player body state(32 - bit)	0 = normal, 1 = invulnerable, 2 = intangible
"453130 2114\n" // Smash attack state	2 = charging smash, 3 = attacking with charged smash, 0 = all other times
"453130 2118\n" // Smash attack charge frame counter

/* 0x80453F10 - - - P2 Block - - - */
"453F10 40\n" // Facing Direction	Float.
"453F10 60\n" // Stamina HP Lost / Percentage - Currently Displayed 	Short
"453F10 68\n" // Falls	Int
"453F10 8E\n" // Stocks	Byte
// (Add pointers to this v to index into player)
/* 0x80453F10 0xB0	Pointer to Player Entity (Probably don't need)*/
"453F10 D1C\n" // Total Damage Received	Float.
"453F10 D28\n" // Total Damage Given	Float.

/* 0x80453FC0 - - - P2 character entity data pointer - - - */
"453FC0 10\n" // Action State	The value at this address indicates the action state of the character.Modifying this alone does nothing.
"453FC0 2C\n" // Facing direction	Float. 1 is right, -1 is left
"453FC0 80\n" // Horizontal velocity(self - induced)	Changing this will only have an effect if the character is in the air.
"453FC0 84\n" // Vertical Velocity(self - induced)	Float.Positive = up, negative = down.Changes when the player jumps / falls of his own accord.Does not kill upward.
"453FC0 8C\n" // Horizontal velocity(attack - induced)	Float.Positive = right, negative = left.
"453FC0 90\n" // Vertical velocity(attack - induced)	Float.Positive = up, negative = down.Only works when player is in the air.Changes when the player is attacked.Kills upward.
"453FC0 B0\n" // X(Horizontal) position	Float.Positive is right, negative is left. 00000000 is center.
"453FC0 B4\n" // Y(Vertical) position	Float.Only works when player is in the air.
"453FC0 C8\n" // X Delta(Curr - Prev Pos)	Float
"453FC0 CC\n" // Y Delta(Curr - Prev Pos)	Float
"453FC0 E0\n" // Ground / Air state	0x00000000 on ground, 0x00000001 in air.Changing does nothing.
"453FC0 894\n" // Action State Frame Counter	Float, resets to 1 with each new action state.
"453FC0 1968\n" // Number of jumps used(8 - bit)	Gives more jumps if lowered.
"453FC0 198C\n" // Player body state(32 - bit)	0 = normal, 1 = invulnerable, 2 = intangible
"453FC0 2114\n" // Smash attack state	2 = charging smash, 3 = attacking with charged smash, 0 = all other times
"453FC0 2118\n" // Smash attack charge frame counter

// Cursor positions (They are stored in opposite player order!?)
"111826C\n" // P2 Cursor X Position	Floating point ranging from - 35 to 26 (If set outside of the boundaries, moves to closest point within boundaries)
"1118270\n" // P2 Cursor Y Position	Floating point ranging from - 22 to 25 (If set outside of the boundaries, moves to closest point within boundaries)
"1118DEC\n" // P1 Cursor X Position	Floating point ranging from - 35 to 26 (If set outside of the boundaries, moves to closest point within boundaries)
"1118DF0\n" // P1 Cursor Y Position	Floating point ranging from - 22 to 25 (If set outside of the boundaries, moves to closest point within boundaries)

/* 0x8049E6C8 - - -STAGE - - -, 0x778 bytes length (Ends at 0x8049EE3F)*/
"479d30\n" // Scene Controller 00 = Current major; 01 = Pending major; 02 = Previous major; 03 = Current minor
"49E6C8 88\n" // Internal Stage ID	See ID lists for Internal Stage ID
"4D6CAD\n" // Stage Identifier (Can't find it on the spreadsheet, but used in altf4/smashbot)

;
#pragma endregion UtilStrings

std::string hotkey =
"Keys/Load State Slot 1 = `Button R`";

Config::Config(VsType vType)
{
    // Set the vs type
    _vs = vType;

    // Set default values
    _dual_core = _DUAL_CORE_DEFAULT;

    // Do we need to render?
    if (_vs == VsType::Human)
    {
        _gfx = true;
        _fullscreen = true;
    }
    else
    {
        _gfx = _GFX_DEFAULT;
        _fullscreen = _FULLSCREEN_DEFAULT;
    }

    initialized = true;
}

std::string Config::getPlayerPipeConfig(int player)
{
    char buff[256];
    printf("%s:%d\tCreating Keyboard Player: %d\n", FILENM, __LINE__, player + 1);
    sprintf(buff, "[GCPad%d]\n", player + 1);
    std::string pipeOut(buff);
    pipeOut += PlayerKeyboard;
    return pipeOut;
}

std::string Config::getAIPipeConfig(int player, int pipe_count, std::string id)
{
    printf("%s:%d\tCreating AI Player: %d\n", FILENM, __LINE__, player + 1);
    char buff[256];
    sprintf(buff,
        "[GCPad%d]\nDevice = Pipe/%d/%s%d\n",
        player + 1,
        pipe_count,
        id.c_str(),
        pipe_count);
    std::string pipeOut(buff);
    pipeOut += AIController;
    return pipeOut;
}

std::string Config::getHotkeyINI(int player, int pipe_count, std::string id)
{
    printf("%s:%d\tCreating Hotkey INI on AI %d\n", FILENM, __LINE__, player + 1);
    char buff[256];
    sprintf(buff,
        "[Hotkeys1]\nDevice = Pipe/%d/%s%d\n",
        pipe_count,
        id.c_str(),
        pipe_count);
    std::string pipeOut(buff);
    pipeOut += hotkey;
    return pipeOut;
}

std::string Config::getLocations()
{
    return memlocation;
}

bool Config::IsInitialized()
{
    return initialized;
}

std::string Config::getConfig()
{
    printf("%s:%d\tConstructing Dolphin.INI\n", FILENM, __LINE__);
    char buff[256];
    std::string output = std::string();
    output += 
        "[Core]\n"
        "DSPHLE = True\n"
        "CPUCore = 1\n"
        "OverclockEnable = False\n";

    // True or False to enable and disable "Dual Core" mode respectively.
    sprintf(buff, "CPUThread = %s\n", _dual_core ? "True" : "False");
    output += buff;

    // Graphics Backend, Defaults Direct3D on WIN32 or OpenGL on other platforms.
    sprintf(buff, "GFXBackend = %s\n", _gfx ? _GFX_BACKEND : "Null");
    output += buff;

    // Keep this section last as it transitions out of [Core]
    if (_gfx)
    {
        output +=
            "EmulationSpeed = 1\n"
            "[Video_Settings]\n"
            "InternalResolution = 3\n"
            "ShaderCompilationMode = 2\n"
            "MSAA = 3\n"
            "SSAA = False\n"
            "[Video_Enhancements]\n"
            "MaxAnisotropy = 3\n"
            "[DSP]\n"
            "Backend = XAudio2\n";
    }
    else
    {
        output +=
            "EmulationSpeed = 0\n";
    }

    return output;
}

Config::~Config()
{
    printf("%s:%d\tDestroying CFG\n", FILENM, __LINE__);
}
