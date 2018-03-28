#include <InputReader.h>

/** provides the environment for loading a new level.
 * Everthing happens in the 'restore' call, so if you want
 * some animation to take place during the loading, we'll
 * have to migrate the animation management into an IRQ handler.
 * Notes:
 *  - switching to the LoadingWindow should be done in handle()
 *    or event(), but not in an 'animating' context.
 *  - if we just let 'if (nextLevel) changeLevel()' in the handle()
 *    method, nothing happens unless a key is pressed...
 *
 * A 'listener' mechanism is thus welcome, but it requires that the
 *  GuiEngine can defer window switch after animation stage.
 */
class LoadingWindow : public DownWindow
{
  NOCOPY(LoadingWindow);
protected:
  InputReader **script;
  Window* game;
public:
  LoadingWindow(InputReader **reader) :
    script(reader), game(0) {
  }
  void setgame(Window* gm) {
    iprintf("bound to game @%p",gm);
    game=gm;
  }

  bool load_at_once() {
    char *lname = GameScript::getNextLevel();
    if (!lname) {
      iprintf(">_< no level\n");
      return false;    
    }
    *script = new FileReader(lname); 
    return true;
  }

  void restore() {
    load_at_once();
    Engine::changeWindow(game);
  }

  void release() {
    iprintf("releasing loader");
    GameScript::setNextLevel("\0");
  }
};

class GameWindow : public DownWindow, public iButtonListener, 
		  public iScriptListener, 
		  private UsingResources
{
  NOCOPY(GameWindow);
protected:
  InputReader** script;
  iScript *gs;
  LoadingWindow* loader;
  bool parse_on_demand;

  Resources resources;
public:
  virtual ~GameWindow() { iprintf("command window destroyed");}
  GameWindow(Window *sup, InputReader **dc, LoadingWindow* loadr) :
    script(dc), gs(0),
    loader(loadr), parse_on_demand(false)
  {
    super=sup;
  }

  /* handles GameScript::setNextLevel(), invoked form LevelGun.
   * This will switch to the 'loader' window, suspending all game
   * activity (CmdWindow::release())
   * LoaderWindow will fill in the "in-ram buffer" and then re-
   *  activate this window (see loader->setgame()), thanks to 
   *  CmdWindow::restore().
   */
  virtual void changeLevel() {
    loader->setgame(this);
    Engine::changeWindow(loader);
  }

  virtual void levelReady() {
    parse_on_demand=false;
    GameScript::setNextLevel("");
  }

  virtual bool event(Widget*, uint) {
    return false;
  }
     
  void loadAll() {
    iprintf("loading ");
    try {
      GameObject::setdebug(false);
      while(gs->parsechunk()) {
	if (keysDown()&KEY_START) return;
	iprintf("#");
      }
    } catch (iScriptException &e) {
      iReport::report(e.what());
      iReport::diagnose();
      die(0,0);
    }
  }

  virtual void customizeGameScript() {}
  
  virtual void restore() {
    iprintf("setup game environment");

    gResources->snap(resources);
    if (script) {
      gs=iScript::create(*script);
    }
    *script = 0;
    iprintf("initializing GameScript");
    gs->regListener(this);
    customizeGameScript();
    Engine::sanitize();
    
    if (!parse_on_demand) {
      loadAll();
    } else iprintf("press X/Y to process level script\n");
  }

  virtual void release() {
    iprintf("releasing cmd");
    gResources->restore(resources);
    BG_PALETTE[255]=RGB15(31,31,31);
    if (gs) gs->stop();
    
  }
};
