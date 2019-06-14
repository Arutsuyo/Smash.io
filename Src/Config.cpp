#include "Config.h"
#include <fstream>
#include <iostream>
#include <sstream>

#define _GFX_BACKEND "OGL"
#define _DUAL_CORE_DEFAULT 1
#define _GFX_DEFAULT false
#define _FULLSCREEN_DEFAULT false
#define FILENM "CFG"

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

std::string memlocation = 
// Used
"004530E0\n" // p1 %
"00453F70\n" // p2 %
"004530C0\n" // p1 direction
"00453F50\n" // p2 direction
"00453090\n" // p1 xpos
"00453F20\n" // p2 xpos
"00453094\n" // p1 ypos
"00453F24\n" // p2 ypos
"00453130 70\n" // p1 action state
"00453130 8F4\n" // p1 action frame
"00453FC0 70\n" // p2 action state
"00453FC0 8F4\n" // p2 action frame
"00479d30\n" // current menu
"0111826C\n" // p2 cursor x
"01118270\n" // p2 cursor y
"01118DEC\n" // p1 cursor x
"01118DF0\n" // p1 cursor y
"00c8ee38\n" // stage select x
"00c8ee50\n" // stage select x
"00C8EE3C\n" // stage select y
"00C8EE60\n" // stage select y

// Unused
//"0045310E\n" // p1stock
//"00453F9E\n" // p2stock
//"00453130 20CC\n" // p1 action count
//"00453130 19EC\n" // p1 invicible
//"00453130 19BC\n" // p1 hitlag
//"00453130 23a0\n" // p1 hitstun
//"00453130 2174\n" // p1 charging
//"00453130 19C8\n" // jumps
//"00453130 140\n" // grounded
//"00453130 E0\n" // xair
//"00453130 E4\n" // yair
//"00453130 EC\n" // xattackspd
//"00453130 F0\n" // yattackspd
//"00453130 14C\n" // knockeddown
//"00453FC0 20CC\n" // p2 action count
//"00453FC0 19EC\n" // p2 invicible
//"00453FC0 19BC\n" // p2 hitlag
//"00453FC0 23a0\n" // p2 hitstun
//"00453FC0 2174\n" // p2 charging
//"00453FC0 19C8\n" // p2 jumps
//"00453FC0 140\n" // p2 grounded
//"00453FC0 E0\n" // p2 xair
//"00453FC0 E4\n" // p2 yair
//"00453FC0 EC\n" // p2 xattackspd
//"00453FC0 F0\n" // p2 yattackspd
//"00453FC0 14C\n" // knockeddown
//"003F0E0A\n" // p1 char
//"003F0E2E\n" // p2 char
//"004D6CAD\n" // stage selected
//"00479D60\n" // still under investigation, but i think is repeated
;

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

int Config::getMemlocationLines(){
    int lines = 0;
    std::istringstream s(memlocation);
    std::string temp;
    while( std::getline(s, temp) )
    {
        lines++;
    }
    return lines;
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
