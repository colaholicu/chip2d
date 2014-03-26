#include "stdafx.h"
#include <Windows.h>
#include "d2dwrap.h"
#include <iostream>
#include <fstream>
#include <assert.h>

#include "emucore.h"

u8 chip8_fontset[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip2d::Chip2d()
{
    bStarted = false;
    drawFlag = true;
}

Chip2d::~Chip2d()
{
}

bool Chip2d::Reset()
{  
    // Clear memory from fontset onwards
    ZeroMemory(memory, 4096 * sizeof(u8));
    ZeroMemory(screen, 2048 * sizeof(u8));

    ZeroMemory(stack, 16 * sizeof(u16));
    ZeroMemory(key, 16 * sizeof(u8));
    ZeroMemory(V, 16 * sizeof(u8));

    p = 0x200;    
    i = 0;
    s = 0;

    opcode = 0;
    X = 0;
    Y = 0;
    N = 0;
    NN = 0;
    NNN = 0;

    drawFlag = true;

    sndTmr = dlyTmr = 0;

    memcpy(memory, nvram, 4096 * sizeof(u8));

    return true;
}

bool Chip2d::Save(const char* szFileName)
{
    std::string strFileName(szFileName);

    if (strFileName.empty())
        return false;

    if (strFileName.find(".sav") == std::string::npos)
    {
        strFileName.append(".sav");
    }

    std::ofstream fs(strFileName.c_str(), std::ofstream::out | std::ofstream::binary);
    if (!fs)
        return false;

    /* the way files are saved:
    1. gameSig
    2. p, i, s
    3. dlyTmr, sndTmr
    4. stack
    5. registers
    6. screen
    7. memory
    8. NVRAM
    */

    // write game signature
    if (!fs.write((const char*)&gameSig, 16 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // write p
    if (!fs.write((const char*)&p, sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // write i
    if (!fs.write((const char*)&i, sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // write s
    if (!fs.write((const char*)&s, sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // write dlyTmr
    if (!fs.write((const char*)&dlyTmr, sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // write sndTmr
    if (!fs.write((const char*)&sndTmr, sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // write stack
    if (!fs.write((const char*)&stack, 16 * sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // write regisers
    if (!fs.write((const char*)&V, 16 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // write screen
    if (!fs.write((const char*)&screen, 2048 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // write memory (0x50+ i.e. skip fontset)
    if (!fs.write((const char*)&memory[0x50], (4096 - 80) * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // write NVRAM (all of it)
    if (!fs.write((const char*)&nvram, 4096 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    fs.close();

    return true;
}

bool Chip2d::Load(const char* szFileName)
{
    std::string strFileName(szFileName);

    if (strFileName.empty())
        return false;

    std::ifstream fs(strFileName.c_str(), std::ofstream::in | std::ofstream::binary);
    if (!fs)
        return false;

    /* the way files are loaded:
    1. gameSig
    2. p, i, s
    3. dlyTmr, sndTmr
    4. stack
    5. registers
    6. screen
    7. memory
    8. NVRAM
    */

    u8 gameSigBuf[16] = {0};

    // read game signature
    if (!fs.read((char*)&gameSigBuf, 16 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    if (gameSig[0] && memcmp(&gameSig, &gameSigBuf, 16 * sizeof(u8)))
    {
        fs.close();
        MessageBox(NULL, "Save slot is from a different game than the current one!", "Ooops!", MB_OK);
        return false;
    }

    Initialize();

    // read p
    if (!fs.read((char*)&p, sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // read i
    if (!fs.read((char*)&i, sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // read s
    if (!fs.read((char*)&s, sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // read dlyTmr
    if (!fs.read((char*)&dlyTmr, sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // read sndTmr
    if (!fs.read((char*)&sndTmr, sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // read stack
    if (!fs.read((char*)&stack, 16 * sizeof(u16)))
    {
        fs.close();
        return false;
    }

    // read regisers
    if (!fs.read((char*)&V, 16 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // read screen
    if (!fs.read((char*)&screen, 2048 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // read memory (0x50+ i.e. skip fontset)
    if (!fs.read((char*)&memory[0x50], (4096 - 80) * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    // read NVRAM (all of it)
    if (!fs.read((char*)&nvram, 4096 * sizeof(u8)))
    {
        fs.close();
        return false;
    }

    memcpy(gameSig, &memory[0x200], sizeof(u8) * 16);    

    fs.close();

    bStarted = true;

    return true;
}

bool Chip2d::Initialize()
{   
    ZeroMemory(memory, 4096 * sizeof(u8));
    ZeroMemory(nvram, 4096 * sizeof(u8));
    ZeroMemory(screen, 2048 * sizeof(u8));

    ZeroMemory(stack, 16 * sizeof(u16));
    ZeroMemory(key, 16 * sizeof(u8));
    ZeroMemory(V, 16 * sizeof(u8));
    
    ZeroMemory(gameSig, 16 * sizeof(u8));

    keyMap.clear();

    memcpy(memory, chip8_fontset, 80 * sizeof(u8));

    p = 0x200;    
    i = 0;
    s = 0;

    opcode = 0;
    X = 0;
    Y = 0;
    N = 0;
    NN = 0;
    NNN = 0;

    drawFlag = true;

    sndTmr = dlyTmr = 0;    

    InitKeys();
    
    return true;
}

bool Chip2d::InitKeys()
{
    keyMap[key0] = 0x30;
    keyMap[key1] = 0x31;
    keyMap[key2] = 0x32;
    keyMap[key3] = 0x33;
    keyMap[key4] = 0x34;
    keyMap[key5] = 0x35;
    keyMap[key6] = 0x36;
    keyMap[key7] = 0x37;
    keyMap[key8] = 0x38;
    keyMap[key9] = 0x39;
    keyMap[keyA] = 0x41;
    keyMap[keyB] = 0x42;
    keyMap[keyC] = 0x43;
    keyMap[keyD] = 0x44;
    keyMap[keyE] = 0x45;
    keyMap[keyF] = 0x46;
    return true;
}

bool Chip2d::Draw()
{
    HRESULT hr = S_OK;
    if (!drawFlag)
        return false;

    static D2D1::ColorF colorWhite = D2D1::ColorF(D2D1::ColorF::White);
    static D2D1::ColorF colorBlack = D2D1::ColorF(D2D1::ColorF::Black);
    static D2D1::ColorF colorBlue  = D2D1::ColorF(D2D1::ColorF::Blue); 

    hr = D2DW::beginDraw(true);
    if (hr != S_OK)
        return false;

    // Draw actual ROM data & screen
    if (bStarted)
    {
        for (u8 y = 0; y < 32; ++y)
        {
            for (u8 x = 0; x < 64; ++x)
            {
                D2DW::drawRect(
                    (screen[x + y*64]) ? colorWhite : colorBlack, 
                    x*PX, y*PY, x*PX+PX, y*PY+PY);
            }
        } 

        drawFlag = false;
    } else {
        // Draw blank screen
        D2DW::drawRect(colorBlue, 0.f, 0.f, 65.f*PX, 33.f*PY);
    }

    hr = D2DW::endDraw();
    if (hr != S_OK)
        return false;

    return true;
}

bool Chip2d::Cycle()
{
    if (!decode())
        return false;

    if (dlyTmr > 0)
        --dlyTmr;

    if (sndTmr > 0)
    {
        if (sndTmr == 1)
            MessageBeep(0xFFFFFFFF);
        --sndTmr;
    }

    return true;
}

bool Chip2d::LoadRom(const char* szFileName)
{
    bStarted = false;

    if (!szFileName)
        return false;

    std::ifstream fs(szFileName, std::ifstream::in | std::ifstream::binary);
    if (!fs)
        return false;

    fs.seekg (0, fs.end);
    std::streamoff nLen = fs.tellg();

    fs.seekg (0, fs.beg);

    if (!(fs.read((char*)(&memory[0x200]), nLen)))
    {
        fs.close();
        return false;
    }

    fs.close();

    memcpy(gameSig, &memory[0x200], sizeof(u8) * 16);

    // copy initial state of the game into NVRAM so that on reset
    // we put in the default state of the game
    memcpy(nvram, memory, sizeof(u8) * 4096);

    bStarted = true;

    return true;
}

bool Chip2d::decode()
{    
    opcode = (memory[p] << 8) | memory[p+1];
    X = (opcode & 0x0f00) >> 8;
    Y = (opcode & 0x00f0) >> 4;
    N = opcode & 0x000f;
    NN = opcode & 0x00ff;   
    NNN = opcode & 0x0fff;

    switch (opcode & 0xf000) {
    case 0x0000:
        switch (N)
        {
        case 0x0: // Clears the screen.
            ZeroMemory(screen, 2048 * sizeof(u8));
            drawFlag = true;
            p += 2;
            break;
        case 0xe: // Returns from a subroutine.
            pop();
            break;
        default:
            MessageBox(NULL, "Unknown opcode (op & 0xf000 = 0x0000)! Press OK to QUIT!", "Ooops", MB_OK);
            return false;
        }
        break;
    case 0x1000: // Jumps to address NNN.
        p = NNN;
        break;
    case 0x2000: // Calls subroutine at NNN.
        push(NNN);
        break;
    case 0x3000: // Skips the next instruction if VX equals NN
        p += (V[X] == NN) ? 4 : 2;
        break;
    case 0x4000: // Skips the next instruction if VX doesn't equal NN.
        p += (V[X] != NN) ? 4 : 2;
        break;
    case 0x5000: // Skips the next instruction if VX equals VY.
        p += (V[X] == V[Y]) ? 4 : 2;
        break;
    case 0x6000: // Sets VX to NN.
        V[X] = NN;
        p += 2;
        break;    
    case 0x7000: // Adds NN to VX.
        V[X] += NN;
        p += 2;
        break;
    case 0x8000:
        switch (N)
        {
        case 0x0: // Sets VX to the value of VY.
            V[X] = V[Y];
            p += 2;
            break;
        case 0x1: // Sets VX to VX or VY.
            V[X] |= V[Y];
            p += 2;
            break;
        case 0x2: // Sets VX to VX and VY.
            V[X] &= V[Y];
            p += 2;
            break;
        case 0x3: // Sets VX to VX xor VY.
            V[X] ^= V[Y];
            p += 2;
            break;
        case 0x4: // Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
            V[0xf] = (V[X] + V[Y]) > 0xff;
            V[X] += V[Y];
            p += 2;
            break;
        case 0x5: // VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
            V[0xf] = (V[Y] <= V[X]);
            V[X] -= V[Y];
            p += 2;
            break;
        case 0x6: // Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.
            V[0xf] = V[X] & 1;
            V[X] >>= 1;
            p += 2;
            break;
        case 0x7: // Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
            V[0xf] = (V[Y] >= V[X]);
            V[X] = V[Y] - V[X];
            p += 2;
            break;
        case 0xe: // Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.
            V[0xf] = V[X] >> 7;
            V[X] <<= 1;
            p += 2;
            break;
        default:
            MessageBox(NULL, "Unknown opcode (op & 0xf000 = 0x8000)! Press OK to QUIT!", "Ooops", MB_OK);
            return false;
        }
        break;
    case 0x9000: // Skips the next instruction if VX doesn't equal VY.
        p += (V[X] != V[Y]) ? 4 : 2;
        break;
    case 0xa000: // Sets i to the address NNN.
        i = NNN;
        p += 2;
        break;
    case 0xb000: // Jumps to the address NNN plus V0.
        p = NNN + V[0];
        break;
    case 0xc000: // Sets VX to a random number and NN.
        V[X] = (rand() % 0xff) & NN;
        p += 2;
        break;
    case 0xd000:
        /* Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. 
           Each row of 8 pixels is read as bit-coded (with the most significant bit of each byte displayed 
           on the left) starting from memory location i; i value doesn't change after the execution of this 
           instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset 
           when the sprite is drawn, and to 0 if that doesn't happen.
        */
        DrawSprite();
        break;
    case 0xe000:
        switch (N)
        {
        case 0x01: // Skips the next instruction if the key stored in VX isn't pressed.
            p += key[V[X]] ? 2 : 4;
            break;
        case 0x0e: // Skips the next instruction if the key stored in VX is pressed.
            p += key[V[X]] ? 4 : 2;
            break;        
        default:
            MessageBox(NULL, "Unknown opcode (op & 0xf000 = 0xe000)! Press OK to QUIT!", "Ooops", MB_OK);
            return false;
        }
        break;
    case 0xf000:
        {
            switch (NN)
            {
            case 0x0007: // VX is set to the delay timer.
                V[X] = dlyTmr;
                p += 2;
                break;
            case 0x000a: // A key press is awaited, and then stored in VX.
                {
                    bool keyPressed = false;
                    for (u8 k = 0; k < 0xf; ++k)
                    {
                        if (key[k])
                        {
                            V[X] = k;
                            keyPressed = true;
                            break;
                        }
                    }
                    
                    if (!keyPressed)
                        return true;

                    p += 2;
                }
                break;
            case 0x0015: // Sets the delay timer to VX.
                dlyTmr = V[X];
                p += 2;
                break;
            case 0x0018: // Sets the sound timer to VX.
                sndTmr = V[X];
                p += 2;
            case 0x001e: 
                /* Adds VX to i
                   VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't. 
                   This is undocumented feature of the Chip-8 and used by Spacefight 2019! game.
                */
                V[0xf] = (i + V[X]) > 0x0fff;
                i += V[X];
                p += 2;
                break;
            case 0x0029: 
                // Sets i to the location of the sprite for the character in VX. 
                // Characters 0-F (in hexadecimal) are represented by a 4x5 font.
                i = V[X] * 0x5;
                p += 2;
                break;
            case 0x0033:
                /* Stores the Binary-coded decimal representation of VX, with the most significant of three digits 
                   at the address in i, the middle digit at i plus 1, and the least significant digit at i plus 2. 
                   (In other words, take the decimal representation of VX, place the hundreds digit in memory at 
                   location in i, the tens digit at location i+1, and the ones digit at location i+2.)
                */
                memory[i] = V[X] / 100;
                memory[i+1] = (V[X] / 10) % 10;
                memory[i+2] = V[X] % 100;
                p += 2;
                break;
            case 0x0055: // Stores V0 to VX in memory starting at address i
                memcpy(&memory[i], V, (X+1) * sizeof(u8));
                p += 2;
                break;
            case 0x0065: // Fills V0 to VX with values from memory starting at address i.
                memcpy(V, &memory[i], (X+1) * sizeof(u8));
                p += 2;
                break;
            default:
                MessageBox(NULL, "Unknown opcode (op & 0xf000 = 0xf000)! Press OK to QUIT!", "Ooops", MB_OK);
                return false;
            }
        }
        break;
    default:
        MessageBox(NULL, "Unknown opcode (op & 0xf000)! Press OK to QUIT!", "Ooops", MB_OK);
        return false;
    }

    return true;
}