struct ast;
struct emitterCtx;

void emitterDecl (struct emitterCtx* ctx, const struct ast* Node);
void emitterDeclStruct (struct emitterCtx* ctx, struct ast* Node);
