#include <vector>
#include <sys/types.h>
#include <nds.h>
#include "SpriteSet.h"
#include "GuiEngine.h"
#include "interfaces.h"
#include "SpriteAnim.h" // cleanup
#include "GameScript.h"
#include "CommonMap.h"
#include "StateMachine.h"
// HOOK #include "farground.h"
#include "LayersConfig.h"
#include "SprFile.h"
#include "ScriptParser.h"
#include "GobExpression.cxx"

extern Engine ge;

#include <cstdarg>
class GameLayers : public LayersConfig {
  unsigned map_index;
  unsigned tile_index;
  int offset;
  unsigned fargroundShape;
public:
  void updateBackgroundTileBase(int offset_)
  {
    offset = offset_;
    iReport::step("assign tilebase %i for bg2\n", offset + tile_index);
    showConsole(console_shown);
  }
  
  GameLayers(unsigned map_index, unsigned tile_index,
	      layersmask_t dontclear = (YOUR << 4) | ALL) :
    LayersConfig(dontclear, default_clearers, Engine::isConsoleDown()),
    map_index(map_index),
    tile_index(tile_index),
    offset(0),
    fargroundShape(BG_64x32)
  {
  }
  void setVerticalFarGround(bool vertical) {
    if (vertical) {
      fargroundShape = BG_32x64;
    } else {
      fargroundShape = BG_64x32;
    }
    showConsole(console_shown);
  }
  
  unsigned getPhysicalLayer(unsigned gameLayer) {
    unsigned physical[4] = { 3, 2, 1, 0 };
    return physical[gameLayer & 3];
  }
  
  void apply() {
    LayersConfig::apply();
    REG_DISPCNT|=DISPLAY_BG3_ACTIVE|DISPLAY_BG2_ACTIVE| // our map
      DISPLAY_BG1_ACTIVE; //< console -- background if any > |DISPLAY_BG    showConsole(console_shown);
    

    REG_BG3CNT=BG_MAP_BASE(iScript::map_index)|BG_TILE_BASE(iScript::tile_index)|BG_PRIORITY(1)
      |BG_COLOR_256|BG_64x32;
    REG_BG2CNT=BG_MAP_BASE(iScript::map_index+2)|BG_TILE_BASE(iScript::tile_index)|BG_PRIORITY(2)
      |BG_COLOR_256|BG_64x32;

    REG_BG0CNT&=~BG_PRIORITY(1);
    REG_BG0CNT|=BG_PRIORITY(2);
    showConsole(console_shown);
    Engine::enable3D();
  }

  void showConsole(visible_t showme) {
    if (showme == SHOW) {
      if (console_down) {
	LayersConfig::showConsole(showme);
	return;
      }
      REG_BG1CNT=BG_MAP_BASE(GECONSOLE)|BG_TILE_BASE(0)|BG_PRIORITY(0); // keep the console on another layer.
      REG_BLDALPHA=0x1008;
      REG_BLDCNT = BLEND_SRC_BG1|
	BLEND_DST_BG2|BLEND_DST_BG0|BLEND_DST_BG3|BLEND_DST_SPRITE|BLEND_DST_BACKDROP|
	BLEND_ALPHA;
      REG_BG1HOFS=0;
      REG_BG1VOFS=0;
    } else {
      // HOOK: restore farground
      REG_BLDCNT = 0;
    }
    console_shown = showme;
  }
};


LayersConfig *UsingGameLayers::gameLayers = 0;
GameLayers* GameScript::gameLayers = 0;

/** any UsingGameLayers item must be able to access it as a 'GameLayers' object.
 * GameScript knows how to create it (has map_index and tile_index value), but
 * also needs to adjust parameters such as the tile base for the background
 * layer.
 */
UsingGameLayers::UsingGameLayers()
{
  if (gameLayers == 0)
    gameLayers = GameScript::makeLayersConfig();
}

UsingGameLayers::~UsingGameLayers()
{
}

LayersConfig *GameScript::makeLayersConfig()
{
  if (gameLayers == 0)
    gameLayers = new GameLayers(map_index, tile_index);
  return gameLayers;
}


#include "InputReader.h"

GameObject* GameScript::getgob(gobno_t i) {
  if (i<MAXGOBS) return objs[i];
  else return 0;
}

GameObject* GameScript::getfocused() {
  /* HOOK : needs a camera */
  return 0;
}

void GameScript::stop() {
  for (uint i=0;i<MAXGOBS;i++) if (objs[i])
    objs[i]->stop();
  for (uint i=0;i<effects.size();i++) effects[i]->stop();
}

GobTank gtk;
template<> GobTank* GobExpression<GobTank>::tank = &gtk;

bool GameScript::reload(InputReader *_input) { 
  return false;
}


GameScript::GameScript(InputReader *ir) : 
  /*script(scr), line(scr)*/
  resources(),
  tiles(), sprites(0),maps(),
  /* - we have 256K video ram allocated 
   * - we're using first tile_index 8K slots for non-game content
   * - each KB can contain 16 tiles (64 bytes each)
   */ 
  tilesram((u16*)BG_MAP_RAM(8*tile_index),(tileno_t)((256-8*tile_index)*16)),
  animsdata((u16*)malloc(256*1024),(tileno_t)(256*1024/64)), /* TODO: improve ram here */
  dynram(), realParser(new ScriptParser(this, ir, gtk, counters, false)),
  effects(), listener(0)
{
  UsingTank::settank(&gtk);
  GobAnim::reset();
  for (uint i=0;i<NB_BGLAYERS;i++) { tiles[i]=0; maps[i]=0; }
  for (uint i=0;i<MAXGOBS;i++) objs[i]=0;
  for (uint i=0;i<MAXSTATES; i++) { states[i]=0; }
  memset(tilesram.raw(),0,64);
  if (running!=0) throw iScriptException("another script running.");
  running=this;
  UsingScript::setscript(this);
  parser = realParser;
}

void GameScript::regListener(iScriptListener *l) {
  listener = l;
}

GameScript::~GameScript() {
  running=0;
  UsingTank::settank(0);
  UsingScript::setscript(0);

  iprintf("cleaning ...\n");
  GameObject::clearDynGobs();
  Animator::ClearAll();

  for (int i=0;i<4;i++) if (tiles[i]) {
    delete tiles[i];
    tiles[i]=0;
  }
  if (sprites) {
    iprintf("sprites, ");
    delete sprites;
  }
  for (int i=0; i < NB_BGLAYERS; i++) {
    if (maps[i]) {
      delete maps[i];
      maps[i]=0;
    }
  }
  iprintf("%i extras, ", effects.size());
  for (unsigned i=0;i<effects.size();i++)
    if (effects[i]) delete effects[i];
  
  { u16 *ad = animsdata.raw();
    iprintf("animsdata @%p",ad);
    if (ad) free(ad);
  }
  
  for (unsigned i=0, n=dynram.size(); i<n; i++) {
    delete dynram[i];
  }
  if (parser && parser == realParser) {
    parser = 0;
    delete realParser;
  }
  gtk.flush();
}

class UseMapAsWorld : UsingWorld {
public:
  UseMapAsWorld(CommonMap *map) {
    world = map;
  }
};


bool GameScript::setMap(bglayer_t bg, bglayer_t mapHolder, unsigned srcplane) {
  if (mapHolder == EMBEDDED) mapHolder = bg;
  {
    CommonMap* map = CommonMap::CreateSimpleMap(tiles[mapHolder],
						(u16*) BG_MAP_RAM(map_index + bg * 2),
						gameLayers->getPhysicalLayer(bg), srcplane);
    gameLayers->setVerticalFarGround(map->needsVertical());
    maps[bg] = map;
    if (bg == 0) { // FIXME: needs the camera system.
      UseMapAsWorld setup(map);
    }
  }
  return true;
}

void GameScript::prepareMap(bglayer_t bg, uint xo, uint yo, bool scrollto) {
  maps[bg]->lookAt(xo,yo);
}

void GameScript::parsedGob(GameObject *gob, unsigned gobno) {
  objs[gobno] = gob;
  gob->run()->regAnim();
}

bool GameScript::parsedFocusRequest(GameObject* gob) {
  // HOOK : need a camera
  return true;
}

void GameScript::parsingDone(char hudmode, s16* herodata) {
  if (listener) listener->levelReady();
}

SpriteSet*
GameScript::loadSprites(const char *fullname, unsigned ramno/*, MedsAnim *animeds out*/)
{
  std::vector<DataBlock> extras;
  extras.clear();
  step("loading %s as sprites... ",fullname);
  Engine::editpal(0,true);
  SpriteRam *ram = new SpriteRam(SPRITE_GFX);
  dynram.push_back(ram);
  sprites=new SpriteSet(ram, SPRITE_PALETTE);
  sprites->ncolors=4096;
  if (!sprites->Load(fullname, ramno, &extras)) {
    throw iScriptException("spriteset not found!");
  }
  if (sprites->getnbtiles()*32>65536) {
    throw iScriptException("0_o too many tiles loaded!");
  }
  Engine::editpal(0,false);
  
  for (int i=0,e=extras.size(); i<e; i++)
    if (extras[i].ptr!=0) free(extras[i].ptr);
  return sprites;
}

SpriteSet* GameScript::loadTiles(const char* fullname, bglayer_t bg)
{
    step("loading %s as tiles for bg %i",fullname,bg);
  
    if (bg<4 && tiles[bg]==0) {
      SpriteRam *ram;
      ram=&tilesram;
      
      tiles[bg]=new SpriteSet(ram,Engine::editpal(bg==2?1:2,true));
      tiles[bg]->ncolors=4096;
      /** only BG3 will have all the palettes. BG2 will repeat palette #0. */
      if (bg==0) tiles[bg]->setextra(&animsdata,1);
      if (! tiles[bg]->Load(fullname) && bg==0) {
	throw iScriptException("tileset not found");
      }
      memset(tilesram.raw(),0,64); // >_< hack.
      if (bg!=2) {
	u16* bg2pal=Engine::editpal(3,true);
	for (uint i=0; i<16; i++)
	  tiles[bg]->setpalette(bg2pal+256*i,0,252);
      }
      Engine::editpal(2,false);
      if (tilesram.size()>1024) {
	throw iScriptException("0_o too many tiles loaded!");
      } else return tiles[bg];
    } else {
      report(">_< invalid tileset number (%i)",bg);
    }
    return 0;
}

GobState* GameScript::parsedGlobalState(GobState *s, unsigned stateno) {
  states[stateno] = s;
  return s;
}

bool GameScript::parsedEffect(Animator* a) {
  effects.push_back(a);
  return true;
}

bool checkTransition()
{ return false; }

bool GameScript::setGobState(gobno_t gobno, unsigned stateno) {
  if (gobno>=MAXGOBS || objs[gobno]==0)
    throw new iScriptException("invalid gob #%i in state assignment",gobno);
  if (stateno>=MAXSTATES || states[stateno]==0)
    throw new iScriptException("invalid state #%i in state assignment",stateno);
  ((CommonGob*)objs[gobno])->switchstate(states[stateno]);
  return true;
}

void subsong(int pp);

/** this one is static */
iScript* iScript::create(InputReader *ir) {
  GameScript *old = GameScript::getrunning();
  GameScript *gs = 0;
  if (old) {
    if (old->reload(ir)) { 
      gs = old;
    } else {
      delete old;
    }
  }
  if (gs == 0) {
    gs = new GameScript(ir);
  }
  return (iScript*)gs;
}
  
iScript* iScript::running() {
  return (iScript*) GameScript::getrunning();
}

/** HOOK for automatic testing */
bool UsingParsing::safecheck() {
  return true;
}

GameScript* GameScript::running=0;
s16 GameScript::counters[17]={};
iScript* UsingScript::script=0;
char GameScript::nextLevel[64];
GameObject* gobForState(GobState *st, enum GameObject::CAST cast, int objno=-1);
