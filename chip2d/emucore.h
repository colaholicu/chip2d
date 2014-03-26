typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;

#include <map>
#include <string>

extern float pixelSizeX;
extern float pixelSizeY;
#define PX (pixelSizeX * 0.8f)
#define PY (pixelSizeY * 0.8f)

enum eKey
{
    key0 = 0, key1, key2, key3, 
    key4, key5, key6, key7, key8, 
    key9, keyA, keyB, keyC, keyD,
    keyE, keyF
};

class Chip2d
{
private:
    u8  memory[4096];
    u8  nvram[4096];

    u8  V[16];

    u16 opcode;
    /*
      X     - reg X
      Y     - reg Y
      N     - 4-bit constant
      NN    - 8-bit constant
      NNN   - address
    */
    u8  X, Y, N, NN;
    u16 NNN;

    u16 i;
    u16 p;

    u16 stack[16];
    u16 s;

    u8  screen[2048];

    u8  sndTmr;
    u8  dlyTmr;

    u8  key[16];

    std::map<eKey, u32> keyMap;

    /* the 1st 16 bytes of the game
    To be used when loading a save game*/
    u8 gameSig[16]; 

    bool bStarted;

protected:

    bool decode();
    inline void push(u16 addr)
    {
        stack[s++] = p;
        p = addr;
    };

    inline void pop()
    {
        p = stack[--s] + 2;
    }

    bool drawFlag;

public:
    Chip2d();
    virtual ~Chip2d();

    bool Initialize();
    bool InitKeys();
    bool LoadRom(const char* szFileName);
    bool Draw();
    inline void DrawSprite()
    {
        u16 x = V[X];
	    u16 y = V[Y];
	    u16 height = N;
	    u16 pixel = 0;

	    V[0xf] = 0;
	    for (int yline = 0; yline < height; yline++)
	    {
		    pixel = memory[i + yline];
		    for (int xline = 0; xline < 8; xline++)
		    {
			    if((pixel & (0x80 >> xline)) != 0)
			    {
				    if(screen[(x + xline + ((y + yline) * 64))] == 1)
				    {
					    V[0xf] = 1;                                    
				    }
				    screen[x + xline + ((y + yline) * 64)] ^= 1;
			    }
		    }
	    }

        drawFlag = true;
        p += 2;
    };

    inline void keyDown(u32 VK)
    {
        auto iter = keyMap.begin();
        for ( ; iter != keyMap.end(); ++iter)
        {
            if (iter->second == VK)
            {
                key[iter->first] = 1;
                return;
            }
        }        
    };
    inline void keyUp(u32 VK)
    {
        auto iter = keyMap.begin();
        for ( ; iter != keyMap.end(); ++iter)
        {
            if (iter->second == VK)
            {
                key[iter->first] = 0;
                return;
            }
        }  
    }

    bool Save(const char* szFileName);
    bool Load(const char* szFileName);

    bool Reset();
    bool Cycle();
    inline bool started()
    {
        return bStarted;
    };
};