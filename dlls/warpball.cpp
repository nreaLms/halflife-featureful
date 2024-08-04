#include <cassert>
#include <cstdio>
#include <map>
#include <vector>
#include <algorithm>
#include <utility>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/schema.h"
#include "rapidjson/error/en.h"

#include "parsetext.h"
#include "extdll.h"
#include "util.h"
#include "color_utils.h"
#include "json_utils.h"
#include "game.h"
#include "effects.h"
#include "soundent.h"
#include "warpball.h"

using namespace rapidjson;

const char warpballCatalogSchema[] = R"(
{
  "type": "object",
  "definitions": {
    "sprite_name": {
      "type": "string",
      "pattern": ".+\\.spr"
    },
    "sprite": {
      "type": ["object", "null"],
      "properties": {
        "sprite": {
          "$ref": "#/definitions/sprite_name"
        },
        "framerate": {
          "type": "number",
          "minimum": 0
        },
        "scale": {
          "type": "number",
          "exclusiveMinimum": 0
        },
        "alpha": {
          "type": "integer",
          "minimum": 0,
          "maximum": 255
        },
        "color": {
          "$ref": "#/definitions/color"
        }
      }
    },
    "sound": {
      "type": ["object", "null"],
      "properties": {
        "sound": {
          "type": "string",
          "pattern": ".+\\.wav"
        },
        "volume": {
          "type": "number",
          "exclusiveMinimum": 0,
          "maximum": 1.0
        },
        "pitch": {
          "$ref": "#/definitions/range"
        },
        "attenuation": {
          "type": "number",
          "minimum": 0
        }
      }
    },
    "alpha": {
      "type": "integer",
      "minimum": 0,
      "maximum": 255
    },
    "color": {
      "type": "string",
      "pattern": "^([0-9]{1,3}[ ]+[0-9]{1,3}[ ]+[0-9]{1,3})|((#|0x)[0-9a-fA-F]{6})$"
    },
    "range": {
      "type": ["string", "object", "number"],
      "pattern": "[0-9]+(\\.[0-9]+)?(,[0-9]+(\\.[0-9]+)?)?",
      "properties": {
        "min": {
          "type": "number"
        },
        "max": {
          "type": "number"
        }
      }
    },
    "range_int": {
      "type": ["string", "object", "number"],
      "pattern": "[0-9]+(,[0-9]+)?",
      "properties": {
        "min": {
          "type": "number"
        },
        "max": {
          "type": "number"
        }
      }
    }
  },
  "properties": {
    "entity_mappings": {
      "type": "object",
      "additionalProperties": {
        "type": "object",
        "additionalProperties": {
          "type": "string",
          "minLength": 1
        }
      }
    },
    "templates": {
      "type": "object",
      "additionalProperties": {
        "type": "object",
        "properties": {
          "inherits": {
            "type": "string"
          },
          "sound1": {
            "$ref": "#/definitions/sound"
          },
          "sound2": {
            "$ref": "#/definitions/sound"
          },
          "sprite1": {
            "$ref": "#/definitions/sprite"
          },
          "sprite2": {
            "$ref": "#/definitions/sprite"
          },
          "beam": {
            "type": "object",
            "properties": {
              "sprite": {
                "$ref": "#/definitions/sprite_name"
              },
              "color": {
                "$ref": "#/definitions/color"
              },
              "alpha": {
                "$ref": "#/definitions/alpha"
              },
              "width": {
                "type": "integer",
                "minimum": 1
              },
              "noise": {
                "type": "integer"
              },
              "life": {
                "$ref": "#/definitions/range"
              }
            }
          },
          "beam_radius": {
            "type": "integer",
            "minumum": 1
          },
          "beam_count": {
            "$ref": "#/definitions/range_int"
          },
          "light": {
            "type": ["object", "null"],
            "properties": {
              "color": {
                "$ref": "#/definitions/color"
              },
              "radius": {
                "type": "integer"
              },
              "life": {
                "type": "number",
                "minimum": 0
              }
            }
          },
          "shake": {
            "type": ["object", "null"],
            "properties": {
              "radius": {
                "type": "integer",
                "minimum": 0
              },
              "duration": {
                "type": "number",
                "minimum": 0.0
              },
              "frequency": {
                "type": "number",
                "exclusiveMinimum": 0,
                "maximum": 255.0
              },
              "amplitude": {
                "type": "number",
                "minimum": 0,
                "maximum": 16
              }
            }
          },
          "ai_sound": {
            "type": ["object", "null"],
            "properties": {
              "type": {
                "type": "string",
                "pattern": "^combat|danger$"
              },
              "duration": {
                "type": "number",
                "minimum": 0
              },
              "radius": {
                "type": "integer",
                "minimum": 0
              }
            }
          },
          "spawn_delay": {
            "type": "number",
            "minimum": 0
          },
          "position": {
            "type": ["object", "null"],
            "properties": {
                "vertical_shift": {
                    "type": "number"
                }
            }
          }
        }
      }
    }
  },
  "required": ["templates"]
}
)";

static float RandomizeNumberFromRange(const FloatRange& r)
{
	if (r.min >= r.max) {
		return r.min;
	}
	return RANDOM_FLOAT(r.min, r.max);
}

static int RandomizeNumberFromRange(const IntRange& r)
{
	if (r.min >= r.max) {
		return r.min;
	}
	return RANDOM_LONG(r.min, r.max);
}

static Color DefaultWarpballColor()
{
	return Color(WARPBALL_RED_DEFAULT, WARPBALL_GREEN_DEFAULT, WARPBALL_BLUE_DEFAULT);
}

static WarpballSprite DefaultWarpballSprite1()
{
	WarpballSprite sprite;
	sprite.sprite = WARPBALL_SPRITE;
	sprite.color = DefaultWarpballColor();
	return sprite;
}

static WarpballSprite DefaultWarpballSprite2()
{
	WarpballSprite sprite;
	sprite.sprite = WARPBALL_SPRITE2;
	sprite.color = DefaultWarpballColor();
	return sprite;
}

static WarpballSound DefaultWarpballSound1()
{
	WarpballSound sound;
	if (g_modFeatures.alien_teleport_sound)
		sound.sound = ALIEN_TELEPORT_SOUND;
	else
		sound.sound = WARPBALL_SOUND1;
	return sound;
}

static WarpballSound DefaultWarpballSound2()
{
	WarpballSound sound;
	if (!g_modFeatures.alien_teleport_sound)
		sound.sound = WARPBALL_SOUND2;
	return sound;
}

static WarpballBeam DefaultWarpballBeam()
{
	WarpballBeam beam;
	beam.sprite = WARPBALL_BEAM;
	beam.color = Color(WARPBALL_BEAM_RED_DEFAULT, WARPBALL_BEAM_GREEN_DEFAULT, WARPBALL_BEAM_BLUE_DEFAULT);
	return beam;
}

static WarpballTemplate DefaultWarpballTemplate()
{
	WarpballTemplate w;
	w.sound1 = DefaultWarpballSound1();
	w.sound2 = DefaultWarpballSound2();
	w.sprite1 = DefaultWarpballSprite1();
	w.sprite2 = DefaultWarpballSprite2();
	w.beam = DefaultWarpballBeam();
	return w;
}

struct WarpballTemplateCatalog
{
	std::map<std::string, std::map<std::string, std::string> > entityMappings;
	std::map<std::string, WarpballTemplate> templates;

	WarpballTemplate* GetWarpballTemplate(const char* warpballName, const char* entityClassname)
	{
		auto warpballTemplate = GetWarpballTemplateByName(warpballName);
		if (warpballTemplate)
			return warpballTemplate;
		if (entityClassname && *entityClassname)
		{
			auto mappingIt = entityMappings.find(warpballName);
			if (mappingIt != entityMappings.end())
			{
				auto& mapping = mappingIt->second;
				auto entityIt = mapping.find(entityClassname);
				if (entityIt != mapping.end())
				{
					warpballTemplate = GetWarpballTemplateByName(entityIt->second.c_str());
					if (warpballTemplate)
						return warpballTemplate;
				}
				auto defaultIt = mapping.find("default");
				if (defaultIt != mapping.end())
				{
					return GetWarpballTemplateByName(defaultIt->second.c_str());
				}
			}
		}
		return nullptr;
	}

private:
	WarpballTemplate* GetWarpballTemplateByName(const char* warpballName)
	{
		auto it = templates.find(warpballName);
		if (it != templates.end())
		{
			return &it->second;
		}
		return nullptr;
	}
};

static void AssignWarpballSound(WarpballSound& sound, Value& soundJson)
{
	if (soundJson.IsNull())
	{
		sound = WarpballSound();
	}
	else
	{
		UpdatePropertyFromJson(sound.sound, soundJson, "sound");
		UpdatePropertyFromJson(sound.volume, soundJson, "volume");
		UpdatePropertyFromJson(sound.pitch, soundJson, "pitch");
		UpdatePropertyFromJson(sound.attenuation, soundJson, "attenuation");
	}
}

static void AssignWarpballSprite(WarpballSprite& sprite, Value& spriteJson)
{
	if (spriteJson.IsNull())
	{
		sprite = WarpballSprite();
	}
	else
	{
		UpdatePropertyFromJson(sprite.sprite, spriteJson, "sprite");
		UpdatePropertyFromJson(sprite.color, spriteJson, "color");
		UpdatePropertyFromJson(sprite.alpha, spriteJson, "alpha");
		UpdatePropertyFromJson(sprite.scale, spriteJson, "scale");
		UpdatePropertyFromJson(sprite.framerate, spriteJson, "framerate");
	}
}

static void AssignWarpballBeam(WarpballBeam& beam, Value& beamJson)
{
	if (beamJson.IsNull())
	{
		beam = WarpballBeam();
	}
	else
	{
		UpdatePropertyFromJson(beam.sprite, beamJson, "sprite");
		UpdatePropertyFromJson(beam.color, beamJson, "color");
		UpdatePropertyFromJson(beam.alpha, beamJson, "alpha");
		UpdatePropertyFromJson(beam.width, beamJson, "width");
		UpdatePropertyFromJson(beam.noise, beamJson, "noise");
		UpdatePropertyFromJson(beam.life, beamJson, "life");
	}
}

static void AssignWarpballLight(WarpballLight& light, Value& lightJson)
{
	if (lightJson.IsNull())
	{
		light = WarpballLight();
	}
	else
	{
		UpdatePropertyFromJson(light.color, lightJson, "color");
		UpdatePropertyFromJson(light.radius, lightJson, "radius");
		UpdatePropertyFromJson(light.life, lightJson, "life");
	}
}

static void AssignWarpballShake(WarpballShake& shake, Value& shakeJson)
{
	if (shakeJson.IsNull())
	{
		shake = WarpballShake();
	}
	else
	{
		UpdatePropertyFromJson(shake.radius, shakeJson, "radius");
		UpdatePropertyFromJson(shake.amplitude, shakeJson, "amplitude");
		UpdatePropertyFromJson(shake.duration, shakeJson, "duration");
		UpdatePropertyFromJson(shake.frequency, shakeJson, "frequency");
	}
}

static void AssignWarpballAiSound(WarpballAiSound& aiSound, Value& aiSoundJson)
{
	if (aiSoundJson.IsNull())
	{
		aiSound = WarpballAiSound();
	}
	else
	{
		UpdatePropertyFromJson(aiSound.radius, aiSoundJson, "radius");
		UpdatePropertyFromJson(aiSound.duration, aiSoundJson, "duration");
		auto it = aiSoundJson.FindMember("type");
		if (it != aiSoundJson.MemberEnd())
		{
			const char* valStr = it->value.GetString();
			if (FStrEq(valStr, "combat"))
			{
				aiSound.type = bits_SOUND_COMBAT;
			}
			else if (FStrEq(valStr, "danger"))
			{
				aiSound.type = bits_SOUND_DANGER;
			}
		}
	}
}

static void AssignWarpballPosition(WarpballPosition& pos, Value& posJson)
{
	if (posJson.IsNull())
	{
		pos = WarpballPosition();
	}
	else
	{
		pos.defined |= UpdatePropertyFromJson(pos.verticalShift, posJson, "vertical_shift");
	}
}

static bool AddWarpballTemplate(WarpballTemplateCatalog& catalog, Value& allTemplatesJsonValue, const char* templateName, Value& templateJsonValue, std::vector<std::string> inheritanceChain = std::vector<std::string>())
{
	if (std::find(inheritanceChain.begin(), inheritanceChain.end(), templateName) != inheritanceChain.end())
	{
		ALERT(at_error, "Cycle in warpball inheritance detected: ");
		for (auto it = inheritanceChain.begin(); it != inheritanceChain.end(); it++)
		{
			ALERT(at_error, "'%s'; ", it->c_str());
		}
		ALERT(at_error, "'%s'\n", templateName);
		return false;
	}

	auto existingTemplateIt = catalog.templates.find(templateName);
	if (existingTemplateIt != catalog.templates.end())
	{
		// Already added, has been used as parent for another template
		return false;
	}

	WarpballTemplate warpballTemplate;
	bool inherited = false;
	auto inheritsIt = templateJsonValue.FindMember("inherits");
	if (inheritsIt != templateJsonValue.MemberEnd())
	{
		const char* parentName = inheritsIt->value.GetString();
		existingTemplateIt = catalog.templates.find(parentName);
		if (existingTemplateIt != catalog.templates.end())
		{
			warpballTemplate = existingTemplateIt->second;
			inherited = true;
		}
		else
		{
			auto parentIt = allTemplatesJsonValue.FindMember(parentName);
			if (parentIt != allTemplatesJsonValue.MemberEnd())
			{
				inheritanceChain.push_back(templateName);
				if (AddWarpballTemplate(catalog, allTemplatesJsonValue, parentName, parentIt->value, inheritanceChain))
				{
					existingTemplateIt = catalog.templates.find(parentName);
					if (existingTemplateIt != catalog.templates.end())
					{
						warpballTemplate = existingTemplateIt->second;
						inherited = true;
					}
				}
				else
					return false;
			}
			else
			{
				ALERT(at_console, "Couldn't find a parent template '%s' for '%s'\n", parentName, templateName);
				return false;
			}
		}
	}
	if (!inherited)
		warpballTemplate = DefaultWarpballTemplate();
	auto sound1It = templateJsonValue.FindMember("sound1");
	if (sound1It != templateJsonValue.MemberEnd())
	{
		AssignWarpballSound(warpballTemplate.sound1, sound1It->value);
	}
	auto sound2It = templateJsonValue.FindMember("sound2");
	if (sound2It != templateJsonValue.MemberEnd())
	{
		AssignWarpballSound(warpballTemplate.sound2, sound2It->value);
	}
	auto sprite1It = templateJsonValue.FindMember("sprite1");
	if (sprite1It != templateJsonValue.MemberEnd())
	{
		AssignWarpballSprite(warpballTemplate.sprite1, sprite1It->value);
	}
	auto sprite2It = templateJsonValue.FindMember("sprite2");
	if (sprite2It != templateJsonValue.MemberEnd())
	{
		AssignWarpballSprite(warpballTemplate.sprite2, sprite2It->value);
	}
	auto beamIt = templateJsonValue.FindMember("beam");
	if (beamIt != templateJsonValue.MemberEnd())
	{
		AssignWarpballBeam(warpballTemplate.beam, beamIt->value);
	}
	UpdatePropertyFromJson(warpballTemplate.beamRadius, templateJsonValue, "beam_radius");
	UpdatePropertyFromJson(warpballTemplate.beamCount, templateJsonValue, "beam_count");
	auto lightIt = templateJsonValue.FindMember("light");
	if (lightIt != templateJsonValue.MemberEnd())
	{
		AssignWarpballLight(warpballTemplate.light, lightIt->value);
	}
	auto shakeIt = templateJsonValue.FindMember("shake");
	if (shakeIt != templateJsonValue.MemberEnd())
	{
		AssignWarpballShake(warpballTemplate.shake, shakeIt->value);
	}
	auto aiSoundIt = templateJsonValue.FindMember("ai_sound");
	if (aiSoundIt != templateJsonValue.MemberEnd())
	{
		AssignWarpballAiSound(warpballTemplate.aiSound, aiSoundIt->value);
	}
	auto positionIt = templateJsonValue.FindMember("position");
	if (positionIt != templateJsonValue.MemberEnd())
	{
		AssignWarpballPosition(warpballTemplate.position, positionIt->value);
	}
	UpdatePropertyFromJson(warpballTemplate.spawnDelay, templateJsonValue, "spawn_delay");
	catalog.templates[templateName] = warpballTemplate;
	return true;
}

WarpballTemplateCatalog g_WarpballCatalog;

void LoadWarpballTemplates()
{
	const char* fileName = "templates/warpball.json";
	int fileSize;
	char *pMemFile = (char*)g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return;

	ALERT(at_console, "Parsing %s\n", fileName);

	Document document;
	bool success = ReadJsonDocumentWithSchema(document, pMemFile, fileSize, warpballCatalogSchema);
	g_engfuncs.pfnFreeFile(pMemFile);

	if (!success)
		return;

	auto templatesIt = document.FindMember("templates");
	if (templatesIt != document.MemberEnd())
	{
		auto& templates = templatesIt->value;
		for (auto templateIt = templates.MemberBegin(); templateIt != templates.MemberEnd(); ++templateIt)
		{
			AddWarpballTemplate(g_WarpballCatalog, templates, templateIt->name.GetString(), templateIt->value);
		}
	}

	auto mappingsIt = document.FindMember("entity_mappings");
	if (mappingsIt != document.MemberEnd())
	{
		auto& entityMappings = mappingsIt->value;
		for (auto mappingIt = entityMappings.MemberBegin(); mappingIt != entityMappings.MemberEnd(); mappingIt++)
		{
			const char* mappingName = mappingIt->name.GetString();
			std::map<std::string, std::string> mapping;
			auto& mappingJson = mappingIt->value;
			for (auto pairIt = mappingJson.MemberBegin(); pairIt != mappingJson.MemberEnd(); ++pairIt)
			{
				auto entityName = pairIt->name.GetString();
				auto warpballName = pairIt->value.GetString();
				if (g_WarpballCatalog.templates.find(warpballName) == g_WarpballCatalog.templates.end())
				{
					ALERT(at_error, "Entity mapping '%s' refers to nonexistent template '%s'\n", mappingName, warpballName);
				}
				mapping[entityName] = warpballName;
			}
			if (mapping.find("default") == mapping.end())
			{
				ALERT(at_error, "Entity mapping '%s' doesn't define 'default' template\n", mappingName);
			}
			else
			{
				g_WarpballCatalog.entityMappings[mappingName] = mapping;
			}
		}
	}
}

static void PrecaheWarpballSprite(WarpballSprite& sprite)
{
	if (sprite.sprite.empty())
	{
		sprite.spriteName = iStringNull;
		return;
	}
	sprite.spriteName = MAKE_STRING(sprite.sprite.c_str());
	PRECACHE_MODEL(STRING(sprite.spriteName));
}

static void PrecacheWarpballSound(WarpballSound& sound)
{
	if (sound.sound.empty())
	{
		sound.soundName = iStringNull;
		return;
	}
	sound.soundName = MAKE_STRING(sound.sound.c_str());
	PRECACHE_SOUND(STRING(sound.soundName));
}

static void PrecacheWarpballBeam(WarpballBeam& beam)
{
	if (beam.sprite.empty())
	{
		beam.spriteName = iStringNull;
		beam.texture = 0;
		return;
	}
	beam.spriteName = MAKE_STRING(beam.sprite.c_str());
	beam.texture = PRECACHE_MODEL(STRING(beam.spriteName));
}

void PrecacheWarpballTemplate(const char* name, const char* entityClassname)
{
	WarpballTemplate* w = g_WarpballCatalog.GetWarpballTemplate(name, entityClassname);
	if (w)
	{
		PrecaheWarpballSprite(w->sprite1);
		PrecaheWarpballSprite(w->sprite2);
		PrecacheWarpballSound(w->sound1);
		PrecacheWarpballSound(w->sound2);
		PrecacheWarpballBeam(w->beam);
	}
}

const WarpballTemplate* FindWarpballTemplate(const char* warpballName, const char* entityClassname)
{
	return g_WarpballCatalog.GetWarpballTemplate(warpballName, entityClassname);
}

static void PlayWarpballSprite(const WarpballSprite& sprite, const Vector& vecOrigin)
{
	if (!FStringNull(sprite.spriteName))
	{
		CSprite *pSpr = CSprite::SpriteCreate( STRING(sprite.spriteName), vecOrigin, TRUE );
		pSpr->AnimateAndDie(sprite.framerate);
		pSpr->SetTransparency(sprite.rendermode, sprite.color.r, sprite.color.g, sprite.color.b, sprite.alpha, sprite.renderfx);
		pSpr->SetScale(sprite.scale > 0 ? sprite.scale : 1.0f);
	}
}

static void PlayWarpballSound(const WarpballSound& sound, const Vector& vecOrigin, edict_t* playSoundEnt)
{
	if (!FStringNull(sound.soundName))
	{
		UTIL_EmitAmbientSound(playSoundEnt, vecOrigin, STRING(sound.soundName), sound.volume, sound.attenuation, 0, RandomizeNumberFromRange(sound.pitch));
	}
}

void PlayWarpballEffect(const WarpballTemplate& warpball, const Vector &vecOrigin, edict_t *playSoundEnt)
{
	PlayWarpballSound(warpball.sound1, vecOrigin, playSoundEnt);
	PlayWarpballSound(warpball.sound2, vecOrigin, playSoundEnt);

	if (warpball.shake.IsDefined())
	{
		auto& shake = warpball.shake;
		UTIL_ScreenShake( vecOrigin, shake.amplitude, shake.frequency, shake.duration, shake.radius );
	}

	PlayWarpballSprite(warpball.sprite1, vecOrigin);
	PlayWarpballSprite(warpball.sprite2, vecOrigin);

	if (warpball.light.IsDefined())
	{
		auto& light = warpball.light;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( vecOrigin.x );	// X
			WRITE_COORD( vecOrigin.y );	// Y
			WRITE_COORD( vecOrigin.z );	// Z
			WRITE_BYTE( (int)(light.radius * 0.1f) );		// radius * 0.1
			WRITE_BYTE( light.color.r );		// r
			WRITE_BYTE( light.color.g );		// g
			WRITE_BYTE( light.color.b );		// b
			WRITE_BYTE( (int)(light.life * 10) );		// time * 10
			WRITE_BYTE( (int)(light.life * 10 / 2) );		// decay * 0.1
		MESSAGE_END();
	}

	auto& beam = warpball.beam;
	if (beam.texture)
	{
		const int iBeams = RandomizeNumberFromRange(warpball.beamCount);
		BeamParams beamParams;
		beamParams.texture = beam.texture;
		beamParams.lifeMin = beam.life.min * 10;
		beamParams.lifeMax = beam.life.max * 10;
		beamParams.width = beam.width;
		beamParams.noise = beam.noise;
		beamParams.red = beam.color.r;
		beamParams.green = beam.color.g;
		beamParams.blue = beam.color.b;
		beamParams.alpha = beam.alpha;
		DrawChaoticBeams(vecOrigin, nullptr, warpball.beamRadius, beamParams, iBeams);
	}

	if (warpball.aiSound.IsDefined())
	{
		auto& aiSound = warpball.aiSound;
		CSoundEnt::InsertSound(aiSound.type, vecOrigin, aiSound.radius, aiSound.duration);
	}
}

static void ReportWarpballSprite(const WarpballSprite& sprite)
{
	if (sprite.sprite.empty()) {
		ALERT(at_console, "undefined\n");
	} else {
		ALERT(at_console, "'%s'. Color: (%d, %d, %d). Alpha: %d. Scale: %g. Framerate: %g\n",
			sprite.sprite.c_str(),
			sprite.color.r, sprite.color.g, sprite.color.b, sprite.alpha,
			sprite.scale, sprite.framerate);
	}
}

static void ReportWarpballSound(const WarpballSound& sound)
{
	if (sound.sound.empty()) {
		ALERT(at_console, "undefined\n");
	} else {
		ALERT(at_console, "'%s'. Volume: %g. Attenuation: %g. Pitch: %d\n", sound.sound.c_str(), sound.volume, sound.attenuation, sound.pitch);
	}
}

static void ReportWarpballBeam(const WarpballBeam& beam)
{
	if (beam.sprite.empty()) {
		ALERT(at_console, "undefined\n");
	} else {
		ALERT(at_console, "'%s. Color: (%d, %d, %d). Alpha: %d. Width: %d. Noise: %d. Life: %g-%g\n",
			beam.sprite.c_str(), beam.color.r, beam.color.g, beam.color.b, beam.alpha,
			beam.width, beam.noise, beam.life.min, beam.life.max);
	}
}

static void ReportWarpballLight(const WarpballLight& light)
{
	if (!light.IsDefined()) {
		ALERT(at_console, "undefined\n");
	} else {
		ALERT(at_console, "Color: (%d, %d, %d). Radius: %d. Life: %g\n", light.color.r, light.color.g, light.color.b, light.radius, light.life);
	}
}

static void ReportWarpballShake(const WarpballShake& shake)
{
	if (!shake.IsDefined()) {
		ALERT(at_console, "undefined\n");
	} else {
		ALERT(at_console, "Amplitude: %d. Frequency: %g. Duration: %g. Radius: %d\n", shake.amplitude, shake.frequency, shake.duration, shake.radius);
	}
}

static void ReportWarpballAiSound(const WarpballAiSound& aiSound)
{
	if (!aiSound.IsDefined()) {
		ALERT(at_console, "undefined\n");
	} else {
		ALERT(at_console, "Type: %d. Radius: %d. Duration: %g\n", aiSound.type, aiSound.radius, aiSound.duration);
	}
}

void DumpWarpballTemplates()
{
	for (auto it = g_WarpballCatalog.templates.begin(); it != g_WarpballCatalog.templates.end(); ++it)
	{
		const WarpballTemplate& w = it->second;
		ALERT(at_console, "Warpball '%s'\n", it->first.c_str());

		ALERT(at_console, "Sprite 1: ");
		ReportWarpballSprite(w.sprite1);
		ALERT(at_console, "Sprite 2: ");
		ReportWarpballSprite(w.sprite2);

		ALERT(at_console, "Sound 1: ");
		ReportWarpballSound(w.sound1);
		ALERT(at_console, "Sound 2: ");
		ReportWarpballSound(w.sound2);

		ALERT(at_console, "Beam: ");
		ReportWarpballBeam(w.beam);

		ALERT(at_console, "Beam radius: %d\n", w.beamRadius);
		ALERT(at_console, "Beam count: %d-%d\n", w.beamCount.min, w.beamCount.max);

		ALERT(at_console, "Light: ");
		ReportWarpballLight(w.light);

		ALERT(at_console, "Shake: ");
		ReportWarpballShake(w.shake);

		ALERT(at_console, "Delay before monster spawn: %g\n", w.spawnDelay);
		if (!w.position.IsDefined())
			ALERT(at_console, "Position: default\n");
		else
			ALERT(at_console, "Position: Vertical shift: %g\n", w.position.verticalShift);

		ALERT(at_console, "\n");
	}
}
