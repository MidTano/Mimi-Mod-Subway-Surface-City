.class Lcom/mod/loader/ModLoader$1$2;
.super Ljava/lang/Object;
.source "ModLoader.java"

# interfaces
.implements Landroid/content/DialogInterface$OnClickListener;


# annotations
.annotation system Ldalvik/annotation/EnclosingMethod;
    value = Lcom/mod/loader/ModLoader$1;->run()V
.end annotation

.annotation system Ldalvik/annotation/InnerClass;
    accessFlags = 0x0
    name = null
.end annotation


# instance fields
.field final synthetic this$0:Lcom/mod/loader/ModLoader$1;

.field final synthetic val$edit:Landroid/widget/EditText;


# direct methods
.method constructor <init>(Lcom/mod/loader/ModLoader$1;Landroid/widget/EditText;)V
    .registers 3
    .annotation system Ldalvik/annotation/MethodParameters;
        accessFlags = {
            0x8010,
            0x1010
        }
        names = {
            null,
            null
        }
    .end annotation

    .annotation system Ldalvik/annotation/Signature;
        value = {
            "()V"
        }
    .end annotation

    .line 61
    iput-object p1, p0, Lcom/mod/loader/ModLoader$1$2;->this$0:Lcom/mod/loader/ModLoader$1;

    iput-object p2, p0, Lcom/mod/loader/ModLoader$1$2;->val$edit:Landroid/widget/EditText;

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method


# virtual methods
.method public onClick(Landroid/content/DialogInterface;I)V
    .registers 3

    .line 63
    iget-object p1, p0, Lcom/mod/loader/ModLoader$1$2;->val$edit:Landroid/widget/EditText;

    invoke-virtual {p1}, Landroid/widget/EditText;->getText()Landroid/text/Editable;

    move-result-object p1

    if-eqz p1, :cond_13

    iget-object p1, p0, Lcom/mod/loader/ModLoader$1$2;->val$edit:Landroid/widget/EditText;

    invoke-virtual {p1}, Landroid/widget/EditText;->getText()Landroid/text/Editable;

    move-result-object p1

    invoke-virtual {p1}, Ljava/lang/Object;->toString()Ljava/lang/String;

    move-result-object p1

    goto :goto_15

    :cond_13
    const-string p1, ""

    :goto_15
    # setter for: Lcom/mod/loader/ModLoader;->sLastSeedInput:Ljava/lang/String;
    invoke-static {p1}, Lcom/mod/loader/ModLoader;->access$102(Ljava/lang/String;)Ljava/lang/String;

    .line 64
    const/4 p1, 0x1

    # setter for: Lcom/mod/loader/ModLoader;->sHasNewSeedInput:Z
    invoke-static {p1}, Lcom/mod/loader/ModLoader;->access$202(Z)Z

    .line 65
    return-void
.end method
