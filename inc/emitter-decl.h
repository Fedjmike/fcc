struct ast;
struct emitterCtx;

void emitterDeclStruct (struct emitterCtx* ctx, struct ast* Node);
void emitterDeclUnion (struct emitterCtx* ctx, struct ast* Node);
void emitterDecl (struct emitterCtx* ctx, const struct ast* Node);
