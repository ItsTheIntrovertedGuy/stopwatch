#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <poll.h>

#include "language_layer.h"
#include "console.c"

#include "font.h"


typedef enum
{
	TIMER_STATE_RUNNING,
	TIMER_STATE_PAUSED,
} timer_state;

typedef enum
{
	FOREGROUND_COLOR_DEFAULT  = 37,
	FOREGROUND_COLOR_RUNNING  = 37,
	FOREGROUND_COLOR_PAUSED   = 34,
} foreground_color;

typedef enum
{
	FONT_INDEX_0 = 0,
	FONT_INDEX_1,
	FONT_INDEX_2,
	FONT_INDEX_3,
	FONT_INDEX_4,
	FONT_INDEX_5,
	FONT_INDEX_6,
	FONT_INDEX_7,
	FONT_INDEX_8,
	FONT_INDEX_9,
	FONT_INDEX_COLON,
	FONT_INDEX_COUNT,
} font_index;

typedef struct
{
	i32 CharWidth;
	i32 CharHeight;
	i32 CharSpacing;
	char *(CharData)[FONT_INDEX_COUNT];
} font_set;

typedef enum 
{
	FONT_SET_SMALL,
	FONT_SET_BIG,
} font_set_type;


global_variable b32 GLOBALConsoleDimensionsChanged = 0;

internal font_set
GetFontSet(font_set_type FontSetType)
{
	font_set Result = { 0 };
	switch (FontSetType)
	{
		case FONT_SET_SMALL: {
			Result.CharWidth   = 1;
			Result.CharHeight  = 1;
			Result.CharSpacing = 0;
			Result.CharData[FONT_INDEX_0]     = GLOBALFontSmall[FONT_INDEX_0];
			Result.CharData[FONT_INDEX_1]     = GLOBALFontSmall[FONT_INDEX_1];
			Result.CharData[FONT_INDEX_2]     = GLOBALFontSmall[FONT_INDEX_2];
			Result.CharData[FONT_INDEX_3]     = GLOBALFontSmall[FONT_INDEX_3];
			Result.CharData[FONT_INDEX_4]     = GLOBALFontSmall[FONT_INDEX_4];
			Result.CharData[FONT_INDEX_5]     = GLOBALFontSmall[FONT_INDEX_5];
			Result.CharData[FONT_INDEX_6]     = GLOBALFontSmall[FONT_INDEX_6];
			Result.CharData[FONT_INDEX_7]     = GLOBALFontSmall[FONT_INDEX_7];
			Result.CharData[FONT_INDEX_8]     = GLOBALFontSmall[FONT_INDEX_8];
			Result.CharData[FONT_INDEX_9]     = GLOBALFontSmall[FONT_INDEX_9];
			Result.CharData[FONT_INDEX_COLON] = GLOBALFontSmall[FONT_INDEX_COLON];
		} break;

		case FONT_SET_BIG: {
			Result.CharWidth   = 7;
			Result.CharHeight  = 6;
			Result.CharSpacing = 1;
			Result.CharData[FONT_INDEX_0]     = GLOBALFontBig[FONT_INDEX_0];
			Result.CharData[FONT_INDEX_1]     = GLOBALFontBig[FONT_INDEX_1];
			Result.CharData[FONT_INDEX_2]     = GLOBALFontBig[FONT_INDEX_2];
			Result.CharData[FONT_INDEX_3]     = GLOBALFontBig[FONT_INDEX_3];
			Result.CharData[FONT_INDEX_4]     = GLOBALFontBig[FONT_INDEX_4];
			Result.CharData[FONT_INDEX_5]     = GLOBALFontBig[FONT_INDEX_5];
			Result.CharData[FONT_INDEX_6]     = GLOBALFontBig[FONT_INDEX_6];
			Result.CharData[FONT_INDEX_7]     = GLOBALFontBig[FONT_INDEX_7];
			Result.CharData[FONT_INDEX_8]     = GLOBALFontBig[FONT_INDEX_8];
			Result.CharData[FONT_INDEX_9]     = GLOBALFontBig[FONT_INDEX_9];
			Result.CharData[FONT_INDEX_COLON] = GLOBALFontBig[FONT_INDEX_COLON];
		} break;
	}
	return (Result);
}

internal font_set_type
GetFontSetTypeFromConsoleDimensions(i32 ConsoleRows, i32 ConsoleColumns)
{
	font_set_type Result = FONT_SET_SMALL;
	enum {
		STRING_CHARS  = 8,
		SPACING_CHARS = 7
	};
	enum {
		MINIMUM_WIDTH_FONT_BIG  = STRING_CHARS*7 + SPACING_CHARS*1,
		MINIMUM_HEIGHT_FONT_BIG = 6,
	};

	if (ConsoleRows    >= MINIMUM_HEIGHT_FONT_BIG && 
		ConsoleColumns >= MINIMUM_WIDTH_FONT_BIG)
	{
		Result = FONT_SET_BIG;
	}

	return (Result);
}

internal void
DrawCharacter(font_set Font, char CharacterToDraw, 
			  i32 DrawPositionY, i32 DrawPositionX)
{
	font_index FontIndex = 0;
	{
		if (CharIsNumber(CharacterToDraw))
		{
			FontIndex = CharacterToDraw - '0';
		}
		else
		{
			Assert(CharacterToDraw == ':');
			FontIndex = FONT_INDEX_COLON;
		}
	}
	
	for (i32 YOffset = 0; YOffset < Font.CharHeight; ++YOffset)
	{
		CursorMove(DrawPositionY + YOffset, DrawPositionX);
		printf("%.*s", Font.CharWidth, Font.CharData[FontIndex] + YOffset*Font.CharWidth);
	}
}

internal void
ConsoleSetForegroundColor(foreground_color ForegroundColor)
{
	enum { BACKGROUND_COLOR_DEFAULT = 40 };
	printf("\033[%d;%dm", ForegroundColor, BACKGROUND_COLOR_DEFAULT);
}

internal void
ConsoleSetup(void)
{
	CursorHide();
	ScreenClear();
	EchoDisable();
}

internal void
ConsoleCleanup(void)
{
	ConsoleSetForegroundColor(FOREGROUND_COLOR_DEFAULT);
	ScreenClear();
	CursorShow();
	CursorMove(0, 0);
	EchoEnable();
}

internal void
SignalSIGINTHandler(int IgnoredSignal)
{
	ConsoleCleanup();
	exit(0);
}

internal void
SignalSIGWINCHHandler(int IgnoredSignal)
{
	GLOBALConsoleDimensionsChanged = 1;
}

internal time_t
GetElapsedTimeInSeconds(struct timeval TimeStampStart)
{
	struct timeval TimeStampCurrent = { 0 };
	(void)gettimeofday(&TimeStampCurrent, 0);
	time_t TotalElapsedSeconds = TimeStampCurrent.tv_sec - TimeStampStart.tv_sec;
	return (TotalElapsedSeconds);
}

int
main(i32 ArgumentCount, char **Arguments)
{
	// NOTE(Felix): Disable printf stdout buffering
	setvbuf(stdout, 0, _IONBF, 0);

	// NOTE(Felix): Set signal so a CTRL-C restores console settings
	// Calls (custom) function internal void SignalSIGINTHandler(int Signal)
	// needs <signal.h>
	{
		struct sigaction SignalAction = { 0 };
		SignalAction.sa_handler = &SignalSIGINTHandler;
		sigaction(SIGINT, &SignalAction, 0);
	}
	
	// NOTE(Felix): Handle resize signal so we can reformat the window
	// Calls (custom) function internal void SignalSIGWINCHHandler(int Signal)
	// needs <signal.h>
	{
		struct sigaction SignalAction = { 0 };
		SignalAction.sa_handler = &SignalSIGWINCHHandler;
		sigaction(SIGWINCH, &SignalAction, 0);
	}
	
	// NOTE(Felix): Disable buffering of input, we want to process it immediately
	{
		struct termios TerminalSettings = { 0 };
		tcgetattr(STDIN_FILENO, &TerminalSettings);
		TerminalSettings.c_lflag &= (tcflag_t)~ICANON;
		tcsetattr(STDIN_FILENO, TCSANOW, &TerminalSettings);
	}
	
	i32 ConsoleColumns = 0;
	i32 ConsoleRows    = 0;
	ConsoleUpdateDimensions(&ConsoleRows, &ConsoleColumns);
	ConsoleSetup();
	font_set_type FontSetType = GetFontSetTypeFromConsoleDimensions(ConsoleRows, ConsoleColumns);
	
	time_t SecondsOffset = 0;
	struct timeval TimeStampTimerStart = { 0 };
	(void)gettimeofday(&TimeStampTimerStart, 0);

	timer_state TimerState = TIMER_STATE_PAUSED;
	b32 RunProgram = 1;
	while (RunProgram)
	{
		// NOTE(Felix): Draw
		{
			char DisplayStringBuffer[50] = { 0 };
			i32 DisplayStringLength = 0;
			{
				time_t TotalElapsedSeconds = 0;
				switch (TimerState)
				{
					case TIMER_STATE_RUNNING: {
						TotalElapsedSeconds = SecondsOffset + GetElapsedTimeInSeconds(TimeStampTimerStart);
					} break;
					
					case TIMER_STATE_PAUSED: {
						TotalElapsedSeconds = SecondsOffset;
					} break;
				}

				i32 Hours = (i32)(TotalElapsedSeconds / (60*60));
				i32 Minutes = (i32)((TotalElapsedSeconds / 60) % 60);
				i32 Seconds = (i32)(TotalElapsedSeconds % 60);
				DisplayStringLength = sprintf(DisplayStringBuffer, "%02d:%02d:%02d", Hours, Minutes, Seconds);
			}

			foreground_color ForegroundColor = (TimerState == TIMER_STATE_RUNNING) ? FOREGROUND_COLOR_RUNNING : FOREGROUND_COLOR_PAUSED;
			ConsoleSetForegroundColor(ForegroundColor);
			
			{
				font_set FontSet = GetFontSet(FontSetType);
				i32 DisplayWidth = DisplayStringLength * FontSet.CharWidth + (DisplayStringLength-1) * FontSet.CharSpacing;
				i32 DisplayHeight = FontSet.CharHeight;
				
				i32 StartX = (ConsoleColumns/2) - (DisplayWidth/2);
				i32 StartY = (ConsoleRows/2)    - (DisplayHeight/2);
				
				// Clear lines
				for (i32 IndexY = 0; IndexY < FontSet.CharHeight; ++IndexY)
				{
					CursorMove(0, StartY + IndexY);
					ClearCurrentLine();
				}
				
				i32 DrawPositionX = StartX;
				i32 DrawPositionY = StartY;
				for (i32 CharIndex = 0; CharIndex+1 < DisplayStringLength; ++CharIndex)
				{
					DrawCharacter(FontSet, DisplayStringBuffer[CharIndex], DrawPositionY, DrawPositionX);
					DrawPositionX += FontSet.CharWidth + FontSet.CharSpacing;
				}
				DrawCharacter(FontSet, DisplayStringBuffer[DisplayStringLength-1], DrawPositionY, DrawPositionX);
			}
		}

		// NOTE(Felix): Wait for time elapsed or intterupt
		i32 InputCharacter = 0;
		{
			struct pollfd PollRequest = { 0 };
			PollRequest.fd = STDIN_FILENO;
			PollRequest.events = POLLIN;

			// NOTE(Felix): Wait for either
			//  - Input
			//  - Interrupt of any kind (including resizing of console)
			poll(&PollRequest, 1, 100); // Wait up to roughly 100ms

			if (GLOBALConsoleDimensionsChanged)
			{
				ConsoleUpdateDimensions(&ConsoleRows, &ConsoleColumns);
				FontSetType = GetFontSetTypeFromConsoleDimensions(ConsoleRows, ConsoleColumns);
				GLOBALConsoleDimensionsChanged = 0;
				ScreenClear();
				continue;
			}

			if (PollRequest.revents)
			{
				read(STDIN_FILENO, &InputCharacter, sizeof(InputCharacter));
			}
		}

		// NOTE(Felix): Handle input
		switch (InputCharacter)
		{
			case 'Q':
			case 'q': {
				RunProgram = 0;
			} break;

			// NOTE(Felix): Reset Timer
			case 'r': {
				SecondsOffset = 0;
				(void)gettimeofday(&TimeStampTimerStart, 0);
			} break;

			// NOTE(Felix): (Un)pause Timer
			case ' ': {
				switch (TimerState)
				{
					case TIMER_STATE_RUNNING: {
						SecondsOffset += GetElapsedTimeInSeconds(TimeStampTimerStart);
						TimerState = TIMER_STATE_PAUSED;
					} break;
					
					case TIMER_STATE_PAUSED: {
						(void)gettimeofday(&TimeStampTimerStart, 0);
						TimerState = TIMER_STATE_RUNNING;
					} break;
				}
			} break;

			case 0:
			default: {
				// noop;
			} break;
		}
	}

	ConsoleCleanup();
	return (0);
}
