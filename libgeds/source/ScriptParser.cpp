#include <vector>
#include <sys/types.h>
#include <nds.h>
#include "GameScript.h"
#include "CommonMap.h"
#include "StateMachine.h"
#include "LayersConfig.h"
// hook : background support
#include "SprFile.h"
#include "ScriptParser.h"
extern Engine ge;

#include <cstdarg>
#include "GobExpression.cxx"

ScriptParser::ScriptParser(GameScript *gs, InputReader *input, GobTank &tank,
			   s16 *ctrs, bool reload)
  : gs(gs), input(input), gtk(tank), counters(ctrs),
    suspended(), 
    tiles(), sprites(0),
    maps(), states(), countergun(), objs(), anim(),
    justreload(reload),
    norelpath(true),
    breakpoint(0), hudmode(0),
  workst(), cmdfile("")
{
  for (uint i=0;i<MAXGOBS; i++) { objs[i] = 0; }
  
}

ScriptParser::~ScriptParser() {
  for (unsigned i=0, n=suspended.size(); i<n; i++) {
    delete suspended[i];
  }
  
  if (input) {
    delete input;
  }
  iprintf("ScriptParser deleted.\n");
}

void ScriptParser::normalize_filename(char *full) {
  if (!norelpath) return;
  char* to = strchr(full,'/');
  char* from=strrchr(full,'/');
  if (!from || !to || from==to) return;
  while(*from) {
    *to++=*from++;
  }
  *to=*from;
}

#include "InputReader.h"

char* UsingParsing::content(char *buf) {
  char *data=buf;
  while (*data==' ' || *data=='\t') data++;
  char* comment;
  if ((comment=strchr(data,'#')))
    *comment=0; // truncate at comment ... that also means no '#' in exprs.
  if (*data==0 || *data=='\n') return 0;
  return data;
}

bool ScriptParser::parsechunk() throw(iScriptException){
  breakpoint=0;
  while (parseline() && breakpoint==0) {
  }
  // we will end here iff parseline() returned false.
  iprintf("bk:%c\n",breakpoint ? breakpoint : 'X');
  return breakpoint=='p';
}


bool ScriptParser::BgCommand(const char* l) {
  uint bgu, xo=0, yo=0;
  uint srcbgu;
  char fullname[64];
  strncpy(fullname, default_dir, 32);
  char *filename = fullname+strlen(fullname);
  step("trying bg commands");
  if (siscanf(l,"bg%u.map \"%30[^\"]\" %u %u",&bgu, filename,&xo,&yo)>=2) {
    bglayer_t bg = (bglayer_t) bgu;
    normalize_filename(fullname);
    step("loading map from %.48s ...\n",fullname);
    if (bg<4 && tiles[bg]!=0 && !tiles[bg]->hasmap()) {
      if (!maps[bg] && tiles[bg]->LoadMap(fullname)) {
	step("map attached\n");
	gs->setMap(bg, bg);
	gs->prepareMap(bg, xo, yo);
	maps[bg] = true;
	return true;
      } else report("O_o couldn't load map %s",fullname);
    } else report(">_< invalid tileset number (%i)",bg);
  }
  uint srcplane;
  if (siscanf(l,"bg%u.map = bg%u.map:%u %u %u",
	      &bgu,    &srcbgu,&srcplane,&xo,&yo)==5) {
    bglayer_t bg = (bglayer_t) bgu;
    bglayer_t srcbg = (bglayer_t) srcbgu;
    step("reusing map.\n");
    if (bg<4 && tiles[srcbg]!=0 && !maps[bg]) {
      uint maxlayers = tiles[srcbg]->hasmap();
      if (srcplane<maxlayers) {
	gs->setMap(bg, srcbg, srcplane);
	gs->prepareMap(bg, xo, yo);
	maps[bg] = true;
	return true;
      } else report(">_< wrong sublayer %i (%i)",srcplane,maxlayers);
    } else report(">_< invalid bgs.\n");

  }

  if (justreload) {
    iprintf("skipping [%s]: just reloading", l);
    return false;
  }
  if (siscanf(l,"bg%u.load \"%30[^\"]\"",&bgu,filename)==2) {
    bglayer_t bg = (bglayer_t) bgu;
    normalize_filename(fullname);
    tiles[bg] = gs->loadTiles(fullname, bg);
    return tiles[bg] != 0;
  }
  return false;
}


bool ScriptParser::AnimCommand(const char* l) {
  unsigned anno;
  {
    unsigned p;  
    if (siscanf(l,"anim%u %x {",&anno, &p)==2) {
      pageno_t pageno = (pageno_t) p;
      const SpritePage *pg = sprites->getpage(pageno);
      if (anno<MAXANIMS) {
	if (pg) {
	  if (!anim[anno]) anim[anno]=new GobAnim(pg);
	  anim[anno]->parse(input);
	  step("done");
	  return true;
	} else report(">_< no such sprite page %i (>%i)",pageno,sprites->getnpages());
      } else report(">_< invalid animation #%i",anno);
    }
  }

  return false;
}

GobTransition* ScriptParser::parseTransition(const char* base, int cont, const GobState* stf) {
  char predicate[64]="", axion[64]="";
  unsigned anno=MAXANIMS;
  step("transition: %s, state=%s", base+cont, stf->pname());
  if (cont>0) {
    while (base[cont]==' ') cont++;
    switch(base[cont]) {
    case '[':
      (siscanf(base+cont,"[%64[^]]] (%64[^)]) :anim%u",predicate, axion,&anno)>=2) 
	|| siscanf(base+cont,"[%64[^]]] :anim%u",predicate,&anno);
      step("<cond %s/>", predicate);
      break;
    case '(':
      siscanf(base+cont," (%64[^)]) :anim%u",axion,&anno);
      step("<action %s/>", axion);
      break;
    case ':':
      siscanf(base+cont," :anim%u",&anno);
      step("<anim %i/>",anno);
      break;
    default:
      step("<error/>");
      break;
    }
  }
  /** we'll use the current gun palette for the transisition. */
  GobTransition *t = new GobTransition(stf);
  step("created transition @%p", t);
  if (anno<MAXANIMS) {
    if (anim[anno]) t->use_anim(anim[anno]);
    else if (anno!=MAXANIMS) report("invalid transanim number %i",anno);
  } 
  if (strlen(predicate)) t->parse_pred(predicate);
  if (strlen(axion)) t->parse_action(axion);
  
  return t;
}

bool ScriptParser::StatCommand(const char* l) {
    unsigned stno;
    unsigned anno=MAXANIMS*2;
    int cont=-1;

    /** definition */
    if (siscanf(l,"stat%*c%u :anim%u {",&stno,&anno)==2) {
      if (stno>=MAXSTATES) {
	report(">_< invalid state %u\n",stno);
	return false;
      }
      if (anno>MAXANIMS || !anim[anno]) {
	report(">_< no such anim (%u)\n",anno);
      }
      else if (workst[stno]) report(">_< state %u already defined\n",stno);
      else {
	char myname[6];
	step("processing state %u\n",stno);
	sniprintf(myname,6,"%c%c%u",cmdfile[0],cmdfile[1],stno);
	workst[stno]=new GobState(anim[anno],myname);
	workst[stno]->parse(input);
	return true;
      }
      return false;
    }
    
    
  { 
    /** transition with an area/controller number (event0, found1 ...) */
    unsigned sti, stf, stl;
    cont=-1;
    
    char event[6];
  
    cont = -1;
    stf=0x5e1f;
    if (siscanf(l, "stat%*c%u..%u -> stat%*c%u on %4[faildone]%n", // [%64[^]]] (%64[^)])",
		/**/  &sti,&stl,     &stf,  event, &cont)>=4 ||
	siscanf(l, "stat%*c%u..%u -> self on %4[faildone]%n",
		/**/  &sti,&stl,            event, &cont)>=3) {
      if ((stf!=0x5e1f && !workst[stf]) || !workst[stl])
	return report("invalid states ..%u->%u\n",stl,stf);
      
      GobTransition *t = parseTransition(l, cont, stf==0x5e1f?&GobState::self:workst[stf]);
      if (t==0) return false;
      
      for (unsigned i=sti;i<=stl;i++) {
	if (!workst[i]) continue;
	switch(event[0]) {
	case 'f':
	  workst[i]->addOnfail(t, i==stl);
	  break;
	case 'd':
	  workst[i]->addOndone(t, i==stl);
	  break;
	default:
	  return report("unknown event type '%s'\n",event);
	}
      }
      return true;
    }
  }
  {
    char event[6];
    unsigned sti,stf=~0U;
    cont=-1;
    //    char predicate[64]="", axion[64]="";
    if (siscanf(l,"stat%*c%u -> stat%*c%u on %5[faildone]%n",
		/**/  &sti,    &stf, event, &cont)>=3 ||
        siscanf(l,"stat%*c%u->nil on %5[faildone]%n",
		/**/  &sti,      event, &cont)>=2) {
      if (!workst[sti] || ((stf!=~0U) && !workst[stf]))
	return report("invalid state numbers (%u->%u)\n",sti,stf);
      if (strncmp(event,"fail",4)!=0 && strncmp(event,"done",4)!=0)
	return report("unknown event type %s",event);

      const GobState *tgt = (stf==~0U) ? &GobState::nil : workst[stf];
      GobTransition *t= parseTransition(l,cont,tgt);
      if (event[0]=='f') workst[sti]->addOnfail(t);
      if (event[0]=='d') {
	if (workst[sti]->getanim()->testloop())
	  return report("%s transition on looping animation",event);
	workst[sti]->addOndone(t); // one of these leaked.
      }
      return true;
    }
  }
  return false;
}

GobState* ScriptParser::getstate(unsigned stateno) const {
  if (stateno<MAXSTATES && workst[stateno]) {
    return workst[stateno];
  } else return 0;
}

GameObject* gobForState(GobState *st, enum GameObject::CAST cast, int objno=-1) {
  switch(st->animMode()) {
  case GobAnim::SIMPLE: 
      return GameObject::CreateSimpleGob(cast, st, objno);
      /* HOOK:  case GobAnim::COMPOUND:
	 return new CompoundGob(0, cast, st, objno);
      */
  default:
      throw iScriptException("unknown anim mode");
    }
}

bool ScriptParser::GobCommand(const char *l) {
  unsigned obj,x,y, stateno;
  int args;
  char heroevil='?';

  if ((args=siscanf(l,"gob%u :stat%*c%u (%u,%u) %c",
		    &obj,   &stateno,&x,&y, &heroevil))>=4) {
    step("initializing gob #%i\n",obj);
    if (obj>MAXGOBS || objs[obj]!=0) {
      iprintf("0_o invalid object no %u\n>%s",obj,l);
      // throw iScriptException(">_< invalid object number");
      return false;
    }

    step("creating gob%u at (%u,%u)\n",obj,x,y);
    if (states[stateno]==0) {
      report(">_< no such state %u for gob %u\n", stateno, obj);
      return false;
    }
    GameObject::CAST cast = GameObject::DYNAMIC;
    if (heroevil=='h') cast=GameObject::HERO;
    if (heroevil=='e') cast=GameObject::EVIL;
    
    objs[obj] = gobForState(states[stateno],cast,obj);
    step("object created.\n");
    objs[obj]->setxy(x,y);
    objs[obj]->dump();
    gs->parsedGob(objs[obj], obj);
    return true;
  }
  /* set new world coordinates for the object */
  if (siscanf(l,"gob%u.move (%u,%u)",&obj,&x,&y)==3) {
    if (obj<MAXGOBS && objs[obj]!=0) {
      objs[obj]->setxy(x,y);
    }
    return true;
  }
  if (siscanf(l,"nofreeze gob%u", &obj)==1) {
    if (obj>MAXGOBS || objs[obj]==0) {
      report("0_o no such object to anti-freeze (%u)\n",obj);
      return false;
    }
    objs[obj]->setfreezable(false);
    return true;
  }
  if (siscanf(l,"focus=gob%u",&obj)==1) {
    if (obj>MAXGOBS || objs[obj]==0) {
      report("0_o no such object to focus (%u)\n",obj);
      return false;
    }
    return gs->parsedFocusRequest(objs[obj]);
  }
  return false;
}

bool ScriptParser::CtrCommand(const char* l) {
    int ctrno, val;

    if (siscanf(l,"let c%u = %i",&ctrno,&val)==2) {
      if (ctrno>16) report(">_< no such counter %i\n",ctrno);
      else counters[ctrno]=val;
      return true;
    }

    char msg[32]="";
    if ((siscanf(l,"c%u.print \"%30[^\"]\"",&ctrno, msg))==2) {
      step("reporting counter %u", ctrno);
      if (ctrno<16) {
	iprintf("%s : %u\n", msg, counters[ctrno]);
      }
      return true;
    }

    if (justreload) return true;
    return false;
}


void subsong(int pp);

void ScriptParser::pushScript() {
  suspended.push_back(input);
  for (uint i = 0; i < MAXSTATES; i++)
    if (workst[i]) {
      /* lost_state.push_back(workst[i]); -- in the tank, now */
      workst[i]=0;
    }

  for (uint i=0; i<MAXANIMS; i++)
    if (anim[i]) {
      // lost_anim.push_back(anim[i]); -- in the tank
      anim[i]=0;
    }

}

void ScriptParser::popScript() {
  for (uint i=0; i<MAXANIMS; i++) {
    anim[i]=0;
  }
  for (uint i=0; i<MAXSTATES; i++) {
    if (workst[i])
      workst[i]->freeze();
  }
  input=suspended.back();
  step("resuming read from %p\n", input);
  suspended.pop_back();

};

bool ScriptParser::parseline(bool debug) throw(iScriptException){
  if (!input) {breakpoint='i'; return false; }

  char lb[256];    
  char *l=0;
  if (!input->readline(lb,256)) {
    breakpoint='l'; return false;
  }
  if (!(l=UsingParsing::content(lb))) return true;

  action(l); // this is the action we're taking.

  if (justreload && strchr(l,'{')) {
    do {
      if (!input->readline(lb,256)) break;
      if (!(l=UsingParsing::content(lb))) continue;
    } while (!l || l[0]!='}');
    return true;
  }

  switch(l[0]) {
  case 'a':
  case 'b':
    if (BgCommand(l)) return true;
    if (justreload) return true;
    if (AnimCommand(l)) return true;
  break;


  case 'i':
    {
      if (justreload) return true;
      char fullname[64];
      strncpy(fullname, default_dir,32);
      char *filename = fullname+strlen(fullname);
      
      if (siscanf(l,"input \"%30[^\"]\"", filename)==1) {
	pushScript();
	input = new FileReader(fullname);
	step("now processing %s",fullname);

	char *cmdname = strrchr(filename,'/');
	if (!cmdname) cmdname=filename;
	strncpy(cmdfile, cmdname, sizeof(cmdfile));
	return true;
      }
    }
    {
      unsigned from,to=0xfff;
      int n;

      if((n=siscanf(l,"import stat%*c %u..%u",&from,&to))>=1) {
	if (n==1) to=from;
	if (from>=MAXSTATES || to>=MAXSTATES) 
	  return ( report(">_< invalid states %u..%u",from,to), false );
	
	unsigned src=0;
	for (unsigned i=from; i<=to; i++) {
	  if (states[i])
	    return (report("0_o state%u already defined",i),false);
	  if (!workst[src])
	    return (report("0_o state%u would have no src",i),false);
	  states[i]= gs->parsedGlobalState(workst[src++], i);
	}
	step("%i states imported\n", src);
	return true;
      }
      break; // 'i*'
    }

  case 'e': {
    if (strncmp(l,"end",3)==0) {
      step("end of file.");
      delete input;
      if (suspended.size()>0) {
	popScript();
      } else {
	step("end of script.\n");
	input=0;
	gs->parsingDone(hudmode, objs[0]->cdata);
      }
      return true;
    }
    break;
  }
    /* HOOK  case 'h':
    if (HudCommand(l)) return true;
    break;
    */
  case 'n': // nofreeze handle in Gob
  case 'f': // focus fallback to gob.
  case 'g':
    if (GobCommand(l)) return true;
    break;

  case 'l': // let c*
  case 'c': // counter%.<xxx>
  case 'd': // do ...
    if (CtrCommand(l)) return true;
    {
      unsigned colnum, colval, r,g,b;
      if (siscanf(l,"color %u %2x%2x%2x",&colnum, &r,&g,&b) == 4) {
	colnum &= 255;
	step("col[%i] = %i,%i,%i\n",colnum,r,g,b);
	colval=RGB15(r,g,b);
	BG_PALETTE[colnum]=colval;
	return true;
      }
    }
    break;
    
  case 'p':{
    char msg[30];
    if (siscanf(l,"print \"%30[^\"]\"",msg)==1) {
      iprintf("> %s\n",msg);
      breakpoint='p';
      return true;
    }
    break;
  }

  case 's':
    if (justreload) return true;
    if (StatCommand(l)) return true;
    if (AnimCommand(l)) return true;
    if (strncmp(l,"spr.",4)==0) {
      unsigned ramno=0; /*, bg*/;
      char fullname[64];
      strncpy(fullname, default_dir,32);
      char *filename = fullname+strlen(fullname);      

      if (siscanf(l,"spr.load \"%30[^\"]\":%i",filename,&ramno)>=1) {
	normalize_filename(fullname);
	step("loading %s as sprites... ",fullname);
	sprites = gs->loadSprites(fullname, ramno);
	return sprites != 0;
      }
    }
    break;

  case 'z':
  {
    int potpos;
    if (siscanf(l,"zik.track(%i)",&potpos)==1) {
      subsong(potpos);
      return true;
    }
    break;
  }
  default:
    break;
  }
  report("cannot parse line %s\n",l);
  return false;
}

bool GameScript::parsechunk() throw(iScriptException){
  while (realParser->parseline()/* && !parser->onBreakpoint()*/) {
  }
  // we will end here iff parseline() returned false.
  delete realParser;
  parser = 0;
  realParser = 0;
  return false;
}

const ScriptParser *UsingScriptParser::parser = 0;
