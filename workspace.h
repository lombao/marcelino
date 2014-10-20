

#include "conf.h"


struct item * * workspace_get_wslist(int b);
struct item ** workspace_get_wslist_current();
void workspace_set(int b,struct item * i);
bool workspace_isempty(int b);
struct item * workspace_get_firstitem(int b);
uint32_t workspace_get_currentws();
void workspace_set_currentws(uint32_t b);

void addtoworkspace(struct client *client, uint32_t ws);
void delfromworkspace(struct client *client, uint32_t ws);
void changeworkspace(uint32_t ws);

