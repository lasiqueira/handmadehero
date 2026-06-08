#include <windows.h>
#include <stdint.h>
#include <xinput.h>

// unsigned integers
typedef uint8_t u8;   // 1-byte long unsigned integer
typedef uint16_t u16; // 2-byte long unsigned integer
typedef uint32_t u32; // 4-byte long unsigned integer
typedef uint64_t u64; // 8-byte long unsigned integer

// signed integers
typedef int8_t s8;   // 1-byte long signed integer
typedef int16_t s16; // 2-byte long signed integer
typedef int32_t s32; // 4-byte long signed integer
typedef int64_t s64; // 8-byte long signed integer
typedef s32 b32;

#define internal static
#define local_persist static
#define global_variable static

struct win32_offscreen_buffer 
{
	// NOTE: Pixels are always 32-bits wide, 
    // Memory Order  0x BB GG RR xx
    // Little Endian 0x xx RR GG BB
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
}; 

struct win32_window_dimension
{
	int Width;
	int Height;
};

global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable bool GlobalRunning;
// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) 
{ 
    return (ERROR_DEVICE_NOT_CONNECTED); 
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) 
{  
    return (ERROR_DEVICE_NOT_CONNECTED); 
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibraryA("Xinput1_4.dll");
	
	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("Xinput1_3.dll");
	}

	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("Xinput9_1_0.dll");
	}

	if(XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if(!XInputGetState)
		{
			XInputGetState = XInputGetStateStub;
		}
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if(!XInputSetState)
		{
			XInputSetState = XInputSetStateStub;
		}
	}
	else
	{
		// We still don't have any XInputLibrary
		XInputGetState = XInputGetStateStub;
		XInputSetState = XInputSetStateStub;

		//TODO: Diagnostic
	}
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	u8 *Row = (u8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		u32 *Pixel = (u32 *) Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			// Pixel in memory: BB GG RR XX
			u8 Red = 0;
			u8 Green = (u8)(Y + YOffset);
			u8 Blue = (u8)(X + XOffset);
	
			*Pixel++ = Red << 16 | Green << 8 | Blue;
		}
		Row += Buffer->Pitch;
	}
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	Buffer->Width = Width;
	Buffer->Height = Height;

	// NOTE(casey): When the biHeight field is negative, this is the clue 
	// to Windows to treat this bitmap as top-down, not bottom-up, meaning
	// that the first bytes of the image are the color for the top left 
	// pixel in the bitmap, not the bottom left!
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	
	Buffer->BytesPerPixel = 4;
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
	int BitmapMemorySize = Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext,int WindowWidth, int WindowHeight)
{
	//TODO aspect ratio correction
	StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight, //Destination
		0, 0, Buffer->Width, Buffer->Height, //Source
		Buffer->Memory,
		&Buffer->Info,
		DIB_RGB_COLORS, SRCCOPY
	);
}

LRESULT CALLBACK Win32MainWindowCallback (HWND Window,
							UINT Message,
							WPARAM WParam,
							LPARAM LParam)
{
	LRESULT Result = 0;
	switch (Message)
	{
		case WM_SIZE:
		{
			
		} break;
		case WM_DESTROY:
		{
			GlobalRunning = false;
		} break;
		case WM_CLOSE:
		{
			GlobalRunning = false;
		} break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			bool IsDown = ((LParam & (1 << 31)) == 0);
			bool WasDown = ((LParam & (1 << 30)) != 0);
			u32 VKCode = WParam;

			if(IsDown != WasDown)
			{
				if(VKCode == 'W')
				{
				}
				else if(VKCode == 'A')
				{
				}
				else if(VKCode == 'S')
				{
				}
				else if(VKCode == 'D')
				{
				}
				else if(VKCode == 'Q')
				{
				}
				else if(VKCode == 'E')
				{
				}
				else if(VKCode == VK_DOWN)
				{
				}
				else if(VKCode == VK_UP)
				{
				}
				else if(VKCode == VK_LEFT)
				{
				}
				else if(VKCode == VK_RIGHT)
				{
				}
				else if(VKCode == VK_ESCAPE)
				{
					OutputDebugStringA("ESCAPE: ");
					if(IsDown)
					{
						OutputDebugStringA("IsDown ");
					}
					if(WasDown)
					{
						OutputDebugStringA("WasDown ");
					}
					OutputDebugStringA("\n");
				}
				else if(VKCode == VK_SPACE)
				{
				}

				b32 AltKeyWasDown = (LParam & (1 << 29));
				if((VKCode == VK_F4) && AltKeyWasDown)
				{
					GlobalRunning = false;
				}
			}
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);

			EndPaint(Window, &Paint);
		} break;
		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return Result;
}

int CALLBACK WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{

	Win32LoadXInput();
	WNDCLASSA WindowClass = {};

	// TODO: check if HREDRAW/VREDRAW/OWNDC still matter 
	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	// WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(0,
									  WindowClass.lpszClassName,
									  "Handmade Hero",
									  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
									  CW_USEDEFAULT,
									  CW_USEDEFAULT,
									  CW_USEDEFAULT,
									  CW_USEDEFAULT,
									  0,
									  0,
									  Instance,
									  0);

		if (Window)
		{	
			Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
			HDC DeviceContext = GetDC(Window);
			int XOffset = 0;
			int YOffset = 0;
			GlobalRunning = true;
			while (GlobalRunning)
			{
				MSG Message;

				while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				//TODO: Should we poll this more frequently?
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						//NOTE: Controller is plugged in
						//TODO: See if ControllerState.dwPacketNumber increments too rapidly
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
						bool Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
						bool Left = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
						bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
						bool Start = Pad->wButtons & XINPUT_GAMEPAD_START;
						bool Back = Pad->wButtons & XINPUT_GAMEPAD_BACK;
						bool LeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
						bool RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
						bool A = Pad->wButtons & XINPUT_GAMEPAD_A;
						bool B = Pad->wButtons & XINPUT_GAMEPAD_B;
						bool X = Pad->wButtons & XINPUT_GAMEPAD_X;
						bool Y = Pad->wButtons & XINPUT_GAMEPAD_Y;

						s16 StickX = Pad->sThumbLX;
						s16 StickY = Pad->sThumbLY;
						
						XOffset += StickX >> 12;
						YOffset += StickY >> 12;
						if(A)
						{
							YOffset += 2;
						}
					}
					else
					{
						//NOTE: Controller is not available
					}
				}

				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 60000;
				Vibration.wRightMotorSpeed = 60000;
				XInputSetState(0, &Vibration);

				RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
				++XOffset;

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);

			}
		}
		else
		{
			//TODO: logging
		}
	}
	else
	{
		//TODO: logging
	}
    return (0);
}