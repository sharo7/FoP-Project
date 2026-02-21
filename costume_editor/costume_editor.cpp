#include "sprite_editor.h"

int main(int argc, char* argv[])
{
    const char* image_path = nullptr;
    if (argc > 1) {
        image_path = argv[1];
    }
    costume_editor(image_path);
    return 0;
}
