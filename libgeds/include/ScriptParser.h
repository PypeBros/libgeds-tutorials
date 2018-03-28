#ifndef _SCRIPTPARSER_H
#define _SCRIPTPARSER_H
#include "SpriteSet.h"
#include "GuiEngine.h"
#include "interfaces.h"
#include "SpriteAnim.h" // cleanup


class GameScript;

/** the logic and state required to convert an InputReader into state machines
 *  and configure a GameScript so that we can play the corresponding level.
 */
class ScriptParser : public iReport, private UsingParsing {
  NOCOPY(ScriptParser);
  GameScript *gs;
  InputReader *input;
  GobTank &gtk;
  s16 *counters; // [MAXCOUNTERS]
  std::vector<InputReader*> suspended;
  SpriteSet *tiles[NB_BGLAYERS], *sprites;
  bool maps[NB_BGLAYERS];
  GobState* states[MAXSTATES];
  bool countergun[MAXCOUNTERS];
  GameObject* objs[MAXGOBS];
  GobAnim* anim[MAXANIMS];
  bool justreload;
  bool norelpath; // set to false to avoid the use of relative path.
  char breakpoint;
  char hudmode;
  GobState *workst[MAXSTATES];
  char cmdfile[31];

public:
  ScriptParser(GameScript *gs, InputReader *input, GobTank &tank, s16 *ctrs,
	       bool reload=false);
  virtual ~ScriptParser();

  /** configure UsingParsing settings */
  ScriptParser* setDirectory(const char* dirname, bool norelative)
  {
    default_dir = dirname;
    norelpath = norelative;
    return this;
  }

  bool parsechunk() throw(iScriptException);
  bool parseline(bool debug=false) throw(iScriptException);
  GobState* getstate(unsigned stateno) const;

private:
  void normalize_filename(char *full);
  bool BgCommand(const char* l);
  bool AnimCommand(const char* l);
  GobTransition* parseTransition(const char* base, int cont, const GobState* stf);
  bool StatCommand(const char* l);
  //  bool setGobState(unsigned gobno, unsigned stateno);
  bool HudCommand(const char *l);
  bool GobCommand(const char *l);
  bool CtrCommand(const char* l);
  void pushScript();
  void popScript();
  GobAnim* makeConditionalAnim(unsigned srcno, unsigned altno, unsigned ctrno);
};

#endif
