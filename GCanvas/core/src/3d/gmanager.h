//
// Created by yuantong on 2017/8/1.
//

#ifndef G_CANVAS_GMANAGER_H
#define G_CANVAS_GMANAGER_H

#include "view/grenderer.h"
#include "util/auto_ptr.h"
#include <map>
#include <string>
#include <memory/AutoPtr.h>

//using namespace taobao::g_engine::gcanvas;
using namespace gcanvas;

class GManager{
public:
    GRenderer* findRenderer(std::string key);
    void removeRenderer(std::string key);
    GRenderer* newRenderer(std::string key);
    GManager();
    virtual ~GManager();

private:
    static AutoPtr<GManager> theManager;
    std::map<std::string, GRenderer*> m_renderMap;

public:
    static GManager* getSingleton();
    static void release();
};

#endif //G_CANVAS_GMANAGER_H
