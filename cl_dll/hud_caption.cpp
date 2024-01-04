#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "parsetext.h"
#include <string.h>
#include <stdio.h>

extern cvar_t *cl_subtitles;

DECLARE_MESSAGE( m_Caption, Caption )

DECLARE_COMMAND( m_Caption, DumpCaptions )

int CHudCaption::Init()
{
	captionsInit = false;
	memset(subtitles, 0, sizeof(subtitles));
	memset(profiles, 0, sizeof(profiles));
	memset(captions, 0, sizeof(captions));
	profileCount = captionCount = 0;

	gHUD.AddHudElem( this );
	HOOK_MESSAGE(Caption);
	Reset();

	HOOK_COMMAND( "dump_captions", DumpCaptions );

	return 1;
}

int CHudCaption::VidInit()
{
	if (!captionsInit)
	{
		ParseCaptionsProfilesFile();
		ParseCaptionsFile();
		captionsInit = true;
	}
	m_hVoiceIcon = SPR_Load("sprites/voiceicon.spr");
	voiceIconWidth = SPR_Width(m_hVoiceIcon, 0);
	voiceIconHeight = SPR_Height(m_hVoiceIcon, 0);
	RecalculateLineOffsets();

	return 1;
}

void CHudCaption::Reset()
{
	m_iFlags = 0;
	memset(subtitles, 0, sizeof(subtitles));
	sub_count = 0;
}

#define SUB_START_XPOS (ScreenWidth / 20)
#define SUB_MAX_XPOS (ScreenWidth - ScreenWidth/20)
#define SUB_BORDER_LENGTH (ScreenWidth/160)

int CHudCaption::MsgFunc_Caption(const char *pszName, int iSize, void *pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	Subtitle_t sub;
	memset(&sub, 0, sizeof(sub));

	float holdTime = READ_BYTE();
	int radio = READ_BYTE();

	const char* captionName = READ_STRING();
	const Caption_t* caption = CaptionLookup(captionName);

	if (!caption)
		return 1;

	sub.caption = caption;

	sub.r = caption->profile->r;
	sub.g = caption->profile->g;
	sub.b = caption->profile->b;

	sub.radio = radio != 0;

	if (holdTime <= 0)
	{
		int perceivedLength = 0;
		const char* ptr = caption->message;
		while(*ptr != '\0') {
			if (*ptr >= 0 && *ptr <= 127)
				perceivedLength += 2;
			else
				perceivedLength ++;
			++ptr;
		}
		holdTime = 2 + perceivedLength/32.0f;
	}
	sub.timeLeft = holdTime;
	sub.timeBeforeStart = caption->delay;
	gEngfuncs.Con_DPrintf("New caption: Hold time: %f. Current time: %f\n", sub.timeLeft, gHUD.m_flTime);

	CalculateLineOffsets(sub);

	AddSubtitle(sub);

	return 1;
}

void CHudCaption::AddSubtitle(const Subtitle_t &sub)
{
	if ((unsigned)sub_count < sizeof(subtitles)/sizeof(subtitles[0]))
	{
		subtitles[sub_count] = sub;
		sub_count++;
	}
}

void CHudCaption::CalculateLineOffsets(Subtitle_t &sub)
{
	const char* str = sub.caption->message;
	const int xmax = SUB_MAX_XPOS - SUB_START_XPOS;

	if (CHud::ShouldUseConsoleFont())
	{
		WordBoundary boundaries[sizeof(sub.caption->message)/2];
		unsigned int wordCount = CHud::SplitIntoWordBoundaries(boundaries, sub.caption->message);

		unsigned int startWordIndex = 0;
		for (unsigned int j=0; j<wordCount;)
		{
			const int width = CHud::UtfText::LineWidth(str + boundaries[startWordIndex].wordStart, boundaries[j].wordEnd - boundaries[startWordIndex].wordStart);
			if (width > xmax) {
				if (j == startWordIndex) {
					sub.lineOffsets[sub.lineCount] = boundaries[startWordIndex].wordStart;
					sub.lineEndOffsets[sub.lineCount] = boundaries[startWordIndex].wordEnd;
					sub.lineCount++;

					startWordIndex = ++j;
				} else {
					sub.lineOffsets[sub.lineCount] = boundaries[startWordIndex].wordStart;
					sub.lineEndOffsets[sub.lineCount] = boundaries[j-1].wordEnd;
					sub.lineCount++;

					startWordIndex = j;
				}
			} else {
				if (j == wordCount - 1) {
					sub.lineOffsets[sub.lineCount] = boundaries[startWordIndex].wordStart;
					sub.lineEndOffsets[sub.lineCount] = boundaries[j].wordEnd;
					sub.lineCount++;
				}

				++j;
			}

			if (sub.lineCount >= SUB_MAX_LINES)
				break;
		}
	}
	else
	{
		int lineWidth = 0;
		const char* currentLine = str;
		const char* lastSpace = str;
		do
		{
			if (*str == '\0')
			{
				sub.lineOffsets[sub.lineCount] = currentLine - sub.caption->message;
				sub.lineEndOffsets[sub.lineCount] = str - sub.caption->message;
				sub.lineCount++;
				break;
			}
			lineWidth += gHUD.m_scrinfo.charWidths[(unsigned char)*str];
			if (*str == ' ' || *str == '\n')
			{
				lastSpace = str;
			}
			if (lineWidth > xmax)
			{
				str = lastSpace;
			}
			if (*str == '\n' || lineWidth > xmax)
			{
				sub.lineOffsets[sub.lineCount] = currentLine - sub.caption->message;
				sub.lineEndOffsets[sub.lineCount] = str - sub.caption->message;
				sub.lineCount++;
				lineWidth = 0;
				currentLine = str + 1;
				if (sub.lineCount >= SUB_MAX_LINES)
					break;
			}
			str++;
		}
		while(true);
	}
}

void CHudCaption::RecalculateLineOffsets()
{
	for (int i=0; i<sub_count; ++i)
	{
		subtitles[i].lineCount = 0;
		memset(subtitles[i].lineOffsets, 0, sizeof(subtitles[i].lineOffsets));
		memset(subtitles[i].lineEndOffsets, 0, sizeof(subtitles[i].lineEndOffsets));
		CalculateLineOffsets(subtitles[i]);
	}
}

void CHudCaption::Update(float flTime, float flTimeDelta)
{
	if (!sub_count)
		return;

	int i;
	for (i=0; i<sub_count; ++i)
	{
		if(subtitles[i].timeLeft <= 0)
		{
			for (int j=i; j<sub_count-1; ++j)
			{
				subtitles[j] = subtitles[j+1];
			}
			memset(&subtitles[sub_count-1], 0, sizeof(subtitles[0]));
			sub_count--;
			break;
		}
	}

	for (i=0; i<sub_count; ++i)
	{
		if (subtitles[i].timeBeforeStart <= 0.0f) {
			subtitles[i].timeLeft -= flTimeDelta;
		} else {
			subtitles[i].timeBeforeStart -= flTimeDelta;
			if (subtitles[i].timeBeforeStart <= 0.0f) {
				subtitles[i].timeLeft += subtitles[i].timeBeforeStart;
			} else {
				continue;
			}
		}
	}
}

int CHudCaption::Draw(float flTime)
{
	if (!sub_count)
		return 0;

	const int hudNumberHeight = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).bottom - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).top;

	const int lineHeight = CHud::UtfText::LineHeight();
	const int distanceBetweenSubs = Q_max(lineHeight / 5, 1);

	const int xpos = SUB_START_XPOS;
	const int xmax = SUB_MAX_XPOS;
	int ypos = ScreenHeight - hudNumberHeight - lineHeight - SUB_BORDER_LENGTH;

	int i, j;

	int overallLineCount = 0;
	int maxLineWidth = 0;
	for (i=0; i<sub_count; ++i)
	{
		if (subtitles[i].timeBeforeStart > 0)
			continue;

		overallLineCount += subtitles[i].lineCount;
		for (j=0; j<subtitles[i].lineCount; ++j)
		{
			int lineWidth = 0;

			if (CHud::ShouldUseConsoleFont())
			{
				lineWidth += CHud::UtfText::LineWidth(subtitles[i].caption->message + subtitles[i].lineOffsets[j], subtitles[i].lineEndOffsets[j] - subtitles[i].lineOffsets[j]);
			}
			else
			{
				const char* str = subtitles[i].caption->message + subtitles[i].lineOffsets[j];
				while( str != subtitles[i].caption->message + subtitles[i].lineEndOffsets[j] )
				{
					lineWidth += gHUD.m_scrinfo.charWidths[(unsigned char)*str];
					str++;
				}
			}

			if (lineWidth > maxLineWidth)
				maxLineWidth = lineWidth;
		}
	}

	if (overallLineCount == 0)
		return 0;

	ypos -= overallLineCount * lineHeight + (sub_count-1) * distanceBetweenSubs;

	if (cl_subtitles->value == 0)
		return 0;

	const int width = maxLineWidth + SUB_BORDER_LENGTH*2;
	const int height = overallLineCount * lineHeight + (sub_count-1) * distanceBetweenSubs + SUB_BORDER_LENGTH*2;
	gEngfuncs.pfnFillRGBABlend( xpos - SUB_BORDER_LENGTH, ypos - SUB_BORDER_LENGTH, width, height, 0, 0, 0, 255 * 0.6 );

	for (i=0; i<sub_count; ++i)
	{
		const Subtitle_t& sub = subtitles[i];
		for (j=0; j<sub.lineCount; ++j)
		{
			if (sub.timeBeforeStart > 0.0f) {
				continue;
			}

			if (j == 0 && m_hVoiceIcon != 0 && sub.radio)
			{
				SPR_Set( m_hVoiceIcon, sub.r, sub.g, sub.b );
				SPR_DrawAdditive( 0, xpos-SUB_BORDER_LENGTH-voiceIconWidth-Q_max(voiceIconWidth/8, 1), ypos + lineHeight/2 - voiceIconWidth/2, NULL );
			}

			CHud::UtfText::DrawString( xpos, ypos, xmax, sub.caption->message + sub.lineOffsets[j], sub.r, sub.g, sub.b, sub.lineEndOffsets[j] - sub.lineOffsets[j] );
			ypos += lineHeight;
		}
		ypos += distanceBetweenSubs;
	}
	return 1;
}

void CHudCaption::UserCmd_DumpCaptions()
{
	gEngfuncs.Con_DPrintf("Captions. HUD time: %f\n", gHUD.m_flTime);
	for (int i=0; i<sub_count; ++i)
	{
		const Subtitle_t& subtitle = subtitles[i];
		if (subtitle.caption)
		{
			gEngfuncs.Con_DPrintf("Caption %d: `%s`. Line count: %d. Time left: %f\n",
							i+1, subtitle.caption->message, subtitle.lineCount, subtitle.timeLeft);
		}
	}
}

#define MAX_CAPTION_NAME_LENGTH 31

static bool IsLatinLowerCase(char c)
{
	return c >= 'a' && c <= 'z';
}

static void ParseCaptionColor(const char* pfile, int& i, int length, CaptionProfile_t& profile)
{
	int rgb[3];
	for (int j=0; j<3; ++j)
	{
		SkipSpaces(pfile, i, length);
		rgb[j] = atoi(pfile + i);
		ConsumeNonSpaceCharacters(pfile, i, length);
	}
	profile.r = rgb[0];
	profile.g = rgb[1];
	profile.b = rgb[2];
}

static void ReportParsedCaptionProfile(const CaptionProfile_t& profile)
{
	gEngfuncs.Con_DPrintf("Parsed a caption profile. ID: %c%c. Color: (%d, %d, %d)\n", profile.firstLetter, profile.secondLetter, profile.r, profile.g, profile.b);
}

bool CHudCaption::ParseCaptionsProfilesFile()
{
	const char* fileName = "sound/captions_profiles.txt";
	int length = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &length );

	if( !pfile )
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s.\n", fileName );
		return false;
	}

	int currentIdStart = 0;
	int i = 0;
	while ( i<length )
	{
		if (IsSpaceCharacter(pfile[i]))
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, length);
		}
		else
		{
			currentIdStart = i;
			ConsumeNonSpaceCharacters(pfile, i, length);
			int tokenLength = i-currentIdStart;

			if (tokenLength == 2)
			{
				char firstLetter = *(pfile + currentIdStart);
				char secondLetter = *(pfile + currentIdStart + 1);
				if (IsLatinLowerCase(firstLetter) && IsLatinLowerCase(secondLetter))
				{
					if (profileCount >= CAPTION_PROFILES_MAX)
					{
						gEngfuncs.Con_Printf("Too many caption profiles! Max is %d\n", CAPTION_PROFILES_MAX);
						break;
					}
					else
					{
						CaptionProfile_t* existingProfile = CaptionProfileLookup(firstLetter, secondLetter);
						if (existingProfile)
						{
							gEngfuncs.Con_Printf("Multiple definitions of caption profile with ID '%c%c'! Skipping.\n", firstLetter, secondLetter);
							ConsumeLine(pfile, i, length);
							continue;
						}

						CaptionProfile_t& profile = profiles[profileCount];
						profile.firstLetter = firstLetter;
						profile.secondLetter = secondLetter;

						ParseCaptionColor(pfile, i, length, profile);

						profileCount++;
						ReportParsedCaptionProfile(profile);
					}
				}
				else
				{
					gEngfuncs.Con_Printf("Both characters in caption profile ID must be lowercase latin characters!\n");
					break;
				}
			}
			else
			{
				gEngfuncs.Con_Printf("Caption profile ID must be 2 characters!\n");
				break;
			}
		}
	}
	gEngfuncs.COM_FreeFile(pfile);
	return true;
}

bool CHudCaption::ParseCaptionsFile()
{
	const char* fileName = "sound/captions.txt";
	int length = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &length );

	if( !pfile )
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s.\n", fileName );
		return false;
	}

	char captionName[sizeof(captions[0].name)];

	int currentTokenStart = 0;
	int i = 0;
	while ( i<length )
	{
		if (IsSpaceCharacter(pfile[i]))
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, length);
		}
		else
		{
			currentTokenStart = i;
			ConsumeNonSpaceCharacters(pfile, i, length);
			int tokenLength = i-currentTokenStart;
			if (!tokenLength || tokenLength >= sizeof(captions[0].name))
			{
				gEngfuncs.Con_Printf("invalid caption name length! Max is %d\n", sizeof(captions[0].name)-1);
				ConsumeLine(pfile, i, length);
				continue;
			}

			strncpy(captionName, pfile + currentTokenStart, tokenLength);
			captionName[tokenLength] = '\0';

			// This code is left for compatibility with existing mods. We should define caption profiles in captions_profiles.txt now instead!
			if (tokenLength == 2 && IsLatinLowerCase(captionName[0]) && IsLatinLowerCase(captionName[1]))
			{
				if (profileCount >= CAPTION_PROFILES_MAX)
				{
					ConsumeLine(pfile, i, length);
					gEngfuncs.Con_Printf("Too many caption profiles! Max is %d\n", CAPTION_PROFILES_MAX);
				}
				else
				{
					char firstLetter = captionName[0];
					char secondLetter = captionName[1];
					CaptionProfile_t* existingProfile = CaptionProfileLookup(firstLetter, secondLetter);
					if (existingProfile)
					{
						gEngfuncs.Con_Printf("Redefining caption profile with ID '%c%c'!.\n", firstLetter, secondLetter);
					}

					CaptionProfile_t& profile = existingProfile ? *existingProfile : profiles[profileCount];
					profile.firstLetter = firstLetter;
					profile.secondLetter = secondLetter;

					ParseCaptionColor(pfile, i, length, profile);

					if (!existingProfile)
						profileCount++;
					ReportParsedCaptionProfile(profile);
				}
			}
			//
			else
			{
				if (captionCount >= CAPTIONS_MAX)
				{
					ConsumeLine(pfile, i, length);
					gEngfuncs.Con_Printf("Too many captions! Max is %d\n", CAPTIONS_MAX);
				}
				else
				{
					Caption_t& caption = captions[captionCount];
					strncpy(caption.name, captionName, sizeof(caption.name));

					SkipSpaces(pfile, i, length);
					currentTokenStart = i;
					ConsumeNonSpaceCharacters(pfile, i, length);

					tokenLength = i-currentTokenStart;
					if (tokenLength > 0 && pfile[currentTokenStart] >= '1' && pfile[currentTokenStart] <= '9')
					{
						char numbuf[8];
						strncpy(numbuf, pfile + currentTokenStart, Q_max(tokenLength, sizeof(numbuf)-1));
						numbuf[sizeof(numbuf)-1] = '\0';

						caption.delay = atof(numbuf);

						SkipSpaces(pfile, i, length);
						currentTokenStart = i;
						ConsumeNonSpaceCharacters(pfile, i, length);

						tokenLength = i-currentTokenStart;
					}

					if (tokenLength != 2 || !IsLatinLowerCase(pfile[currentTokenStart]) || !IsLatinLowerCase(pfile[currentTokenStart+1]))
					{
						gEngfuncs.Con_Printf("invalid caption profile for %s! Must be 2 lowercase latin characters\n", caption.name);
						ConsumeLine(pfile, i, length);
						continue;
					}

					char firstLetter = pfile[currentTokenStart];
					char secondLetter = pfile[currentTokenStart+1];
					caption.profile = CaptionProfileLookup(firstLetter, secondLetter);

					if (!caption.profile)
					{
						gEngfuncs.Con_Printf("Could not find a caption profile %c%c for %s\n", firstLetter, secondLetter, caption.name);
						ConsumeLine(pfile, i, length);
						continue;
					}

					SkipSpaces(pfile, i, length);
					currentTokenStart = i;
					ConsumeLine(pfile, i, length);

					tokenLength = i-currentTokenStart;

					if (!tokenLength || tokenLength >= sizeof(caption.message))
					{
						gEngfuncs.Con_Printf("Invalid caption message length for %s! Max is %d\n", caption.name, sizeof(caption.message)-1);
						continue;
					}

					strncpy(caption.message, pfile + currentTokenStart, tokenLength);
					caption.message[tokenLength] = '\0';

					captionCount++;
					//gEngfuncs.Con_DPrintf("Parsed a caption. Name: %s. Profile: %c%c. Text: %s\n", caption.name, caption.profile->firstLetter, caption.profile->secondLetter, caption.message);
				}
			}
		}
	}
	SortCaptions();

	gEngfuncs.COM_FreeFile(pfile);
	return true;
}

void CHudCaption::SortCaptions()
{
	int i, j;

	for( i = 0; i < captionCount; i++ )
	{
		for( j = i + 1; j < captionCount; j++ )
		{
			if( strcmp( captions[i].name, captions[j].name ) > 0 )
			{
				Caption_t tmp = captions[i];
				captions[i] = captions[j];
				captions[j] = tmp;
			}
		}
	}
}

const Caption_t* CHudCaption::CaptionLookup(const char *name)
{
	int left, right, pivot;
	int val;

	left = 0;
	right = captionCount - 1;

	while( left <= right )
	{
		pivot = ( left + right ) / 2;

		val = strcmp( name, captions[pivot].name );
		if( val == 0 )
		{
			return &captions[pivot];
		}
		else if( val > 0 )
		{
			left = pivot + 1;
		}
		else if( val < 0 )
		{
			right = pivot - 1;
		}
	}
	return NULL;
}

CaptionProfile_t* CHudCaption::CaptionProfileLookup(char firstLetter, char secondLetter)
{
	for (int k=0; k<profileCount; ++k)
	{
		if (profiles[k].firstLetter == firstLetter && profiles[k].secondLetter == secondLetter)
		{
			return &profiles[k];
		}
	}
	return NULL;
}
