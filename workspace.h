



struct item * * workspace_get(int b);
void workspace_set(int b,struct item * i);
bool workspace_isempty(int b);
struct item * workspace_get_firstitem(int b);

void addtoworkspace(struct client *client, uint32_t ws);
void delfromworkspace(struct client *client, uint32_t ws);
void changeworkspace(uint32_t ws);

