/* -*- C++ -*- */
#ifndef __GAMESCRIPT_H
#define __GAMESCRIPT_H
#include <vector>
#include <map>
#include "interfaces.h"
#include "SpriteSet.h"
class GameObject; 
class GobState; class GobAnim; class GobStructure; 
class CommonMap;

class ScriptParser;
#include <GameConstants.h>

class UsingScriptParser : protected UsingParsing {
protected:
  static const ScriptParser* parser;
};

/** The real running-level state
 *  it lives as long as we are in the level. It owns all the resource containers
 *  as well as all the dynamic GameObjects.
 */
class GameScript : public iScript, public iReport,
		   private UsingResources, private UsingScriptParser
{
  NOCOPY(GameScript);
#ifdef GLOBAL_DEBUGGER
  friend GLOBAL_DEBUGGER;
#endif
  static char nextLevel[64];
private:
  Resources resources;
  SpriteSet *tiles[NB_BGLAYERS],*sprites;

  /* controls scrolling and level layers display */
  CommonMap *maps[NB_BGLAYERS];
  /* we will use the upper 64K of VRAM for our tiles.
   * to be assigned among the max 4 tilesets we will manage.
   */
  SpriteRam tilesram, animsdata;
  std::vector<SpriteRam*> dynram;
  //  GobAnim *anim[MAXANIMS];
  ScriptParser* realParser;
  GameObject *objs[MAXGOBS];
  GobState *states[MAXSTATES];
  static GameScript* running;
  static s16 counters[MAXCOUNTERS+1]; // counter[16] is the score.

  std::vector<Animator*> effects;
  iScriptListener* listener;
  GobTransition* parseTransition(const char* base, int cont, const GobState* stf);

public:
  static void testcontroller(InputReader *script);
  enum loadmode { LOADALL, RELOAD };
  static void setNextLevel(const char *name, enum loadmode loadall=LOADALL) {
    strcpy(nextLevel, name);
    if (strlen(name)==0) return;
    if (!running) { iprintf("0_o no running level!\n"); return; }
    if (!running->listener) { iprintf("0_o no listener!\n"); return; }
    running->listener->changeLevel();
  }

  static char *getNextLevel() {
    static char fullname[64];
    strncpy(fullname, default_dir, 64);
    if (nextLevel[0]) {
      strncpy(fullname + strlen(fullname), nextLevel, 64-strlen(fullname));
      return fullname;
    } else return 0;
  }

  static GameObject* getobj(int no) {
    if (no<16 && running) return running->objs[no];
    else return 0;
  }
  static GobState *getstate(int no) {
    if (no<16 && running) return running->states[no];
    else return 0;
  }

  virtual void regListener(iScriptListener *l);

  static char* content(char* buffer); // trim comments and leading space.

  GameScript(InputReader *input);
  bool reload(InputReader *i);
  static GameScript* getrunning() { return running; }
  bool ParsingDone();
  static GobState* getstate(unsigned i);
  virtual GameObject* getfocused();
  virtual GameObject* getgob(gobno_t no);
  virtual ~GameScript();
  /** parser callbacks interface */
  bool setMap(bglayer_t bg, bglayer_t mapHolder, unsigned mapPlane=0);
  SpriteSet* loadTiles(const char* filename, bglayer_t ly);
  SpriteSet* loadSprites(const char* filename, unsigned ramno);
  void prepareMap(bglayer_t bg, uint xo, uint yo, bool scrollNow = false);
  GobState* parsedGlobalState(GobState* state, unsigned stateno);
  bool parsedEffect(Animator* fx);
  void parsedGob(GameObject* gob, unsigned gobno);
  bool parsedFocusRequest(GameObject* gob);
  void parsingDone(char hudmode, s16 *cdta);
  virtual bool parsechunk() throw(iScriptException);
  virtual void stop();
  SpriteSet* getSpriteSet(bglayer_t i) {
    if (i<4 && tiles[i])
      return tiles[0];
    else if (i==4 && sprites) return sprites;
    else return 0;
  }
  virtual bool setGobState(gobno_t target, unsigned stateno);
};

#endif
