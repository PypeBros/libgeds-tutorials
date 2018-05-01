#include <nds.h>
#include "GuiEngine.h"
#include "LayersConfig.h"

const u16 LayersConfig::default_clearers[] = {127,0,0,0,127,0,0,0};
LayersConfig defaultGuiLayers(LayersConfig::CONSOLE, LayersConfig::default_clearers);
LayersConfig *Window::GuiLayers = &defaultGuiLayers;
