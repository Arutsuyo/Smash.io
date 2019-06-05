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
"Main Stick/Up = `Axis MAIN Y - `\n"
"Main Stick/Down = `Axis MAIN Y + `\n"
"Main Stick/Left = `Axis MAIN X - `\n"
"Main Stick/Right = `Axis MAIN X + `\n"
"C-Stick/Up = `Axis C Y - `\n"
"C-Stick/Down = `Axis C Y + `\n"
"C-Stick/Left = `Axis C X - `\n"
"C-Stick/Right = `Axis C X + `\n";

std::string memlocation =
"004530E0\n"
"00453F70\n"
"0045310E\n"
"00453F9E\n"
"004530C0\n"
"00453F50\n"
"00453090\n"
"00453F20\n"
"00453094\n"
"00453F24\n"
"00453130 70\n"
"00453130 20CC\n"
"00453130 8F4\n"
"00453130 19EC\n"
"00453130 19BC\n"
"00453130 23a0\n"
"00453130 2174\n"
"00453130 19C8\n"
"00453130 140\n"
"00453130 E0\n"
"00453130 E4\n"
"00453130 EC\n"
"00453130 F0\n"
"00453130 14C\n"
"00453FC0 70\n"
"00453FC0 20CC\n"
"00453FC0 8F4\n"
"00453FC0 19EC\n"
"00453FC0 19BC\n"
"00453FC0 23a0\n"
"00453FC0 2174\n"
"00453FC0 19C8\n"
"00453FC0 140\n"
"00453FC0 E0\n"
"00453FC0 E4\n"
"00453FC0 EC\n"
"00453FC0 F0\n"
"00453FC0 14C\n"
"003F0E0A\n"
"003F0E2E\n"
"00479d30\n"
"004D6CAD\n"
"0111826C\n"
"01118270\n"
"00479D60\n"
"\n";
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
