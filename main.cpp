#include "engine.h"

int main(int argc, char* argv[]) {
    Engine app;
    if (!app.Initialize()) {
        return -1;
    }
    app.Run();
    return 0;
}
