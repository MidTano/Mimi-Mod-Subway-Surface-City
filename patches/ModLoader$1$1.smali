.class Lcom/mod/loader/ModLoader$1$1;
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


# direct methods
.method constructor <init>(Lcom/mod/loader/ModLoader$1;)V
    .registers 2
    .annotation system Ldalvik/annotation/MethodParameters;
        accessFlags = {
            0x8010
        }
        names = {
            null
        }
    .end annotation

    .line 67
    iput-object p1, p0, Lcom/mod/loader/ModLoader$1$1;->this$0:Lcom/mod/loader/ModLoader$1;

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method


# virtual methods
.method public onClick(Landroid/content/DialogInterface;I)V
    .registers 3

    .line 69
    const-string p1, ""

    # setter for: Lcom/mod/loader/ModLoader;->sLastSeedInput:Ljava/lang/String;
    invoke-static {p1}, Lcom/mod/loader/ModLoader;->access$102(Ljava/lang/String;)Ljava/lang/String;

    .line 70
    const/4 p1, 0x1

    # setter for: Lcom/mod/loader/ModLoader;->sHasNewSeedInput:Z
    invoke-static {p1}, Lcom/mod/loader/ModLoader;->access$202(Z)Z

    .line 71
    return-void
.end method
