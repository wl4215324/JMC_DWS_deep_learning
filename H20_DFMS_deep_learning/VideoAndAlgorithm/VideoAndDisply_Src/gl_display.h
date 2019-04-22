#ifndef __GL_DISPLAY_H__
#define __GL_DISPLAY_H__

class FbDisplay
{
public:
    FbDisplay();
    ~FbDisplay();
    int display_buffer(char *buffer);
private:
};

int display_init();
int display_buffer(char *buffer);
void* disp_image(void* argv);

#endif
