.class Lcom/mod/loader/ModLoader$1;
.super Ljava/lang/Object;
.source "ModLoader.java"

# interfaces
.implements Ljava/lang/Runnable;


# annotations
.annotation system Ldalvik/annotation/EnclosingMethod;
    value = Lcom/mod/loader/ModLoader;->promptSeedInput(Ljava/lang/String;)V
.end annotation

.annotation system Ldalvik/annotation/InnerClass;
    accessFlags = 0x0
    name = null
.end annotation


# instance fields
.field final synthetic val$activity:Landroid/app/Activity;

.field final synthetic val$current:Ljava/lang/String;


# direct methods
.method constructor <init>(Landroid/app/Activity;Ljava/lang/String;)V
    .registers 3
    .annotation system Ldalvik/annotation/Signature;
        value = {
            "()V"
        }
    .end annotation

    .line 33
    iput-object p1, p0, Lcom/mod/loader/ModLoader$1;->val$activity:Landroid/app/Activity;

    iput-object p2, p0, Lcom/mod/loader/ModLoader$1;->val$current:Ljava/lang/String;

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method


# virtual methods
.method public run()V
    .registers 7

    .line 36
    :try_start_0
    iget-object v0, p0, Lcom/mod/loader/ModLoader$1;->val$activity:Landroid/app/Activity;

    .line 37
    new-instance v1, Landroid/widget/LinearLayout;

    invoke-direct {v1, v0}, Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V

    .line 38
    const/4 v2, 0x1

    invoke-virtual {v1, v2}, Landroid/widget/LinearLayout;->setOrientation(I)V

    .line 39
    const/16 v2, 0x10

    # invokes: Lcom/mod/loader/ModLoader;->dp(Landroid/content/Context;I)I
    invoke-static {v0, v2}, Lcom/mod/loader/ModLoader;->access$000(Landroid/content/Context;I)I

    move-result v2

    .line 40
    invoke-virtual {v1, v2, v2, v2, v2}, Landroid/widget/LinearLayout;->setPadding(IIII)V

    .line 42
    new-instance v2, Landroid/widget/TextView;

    invoke-direct {v2, v0}, Landroid/widget/TextView;-><init>(Landroid/content/Context;)V

    .line 43
    const-string v3, "enter seed (digits, empty = random)"

    invoke-virtual {v2, v3}, Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V

    .line 44
    const/high16 v3, 0x41400000    # 12.0f

    const/4 v4, 0x2

    invoke-virtual {v2, v4, v3}, Landroid/widget/TextView;->setTextSize(IF)V

    .line 45
    invoke-virtual {v1, v2}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V

    .line 47
    new-instance v2, Landroid/widget/EditText;

    invoke-direct {v2, v0}, Landroid/widget/EditText;-><init>(Landroid/content/Context;)V

    .line 48
    invoke-virtual {v2, v4}, Landroid/widget/EditText;->setInputType(I)V

    .line 49
    iget-object v3, p0, Lcom/mod/loader/ModLoader$1;->val$current:Ljava/lang/String;

    if-eqz v3, :cond_36

    iget-object v3, p0, Lcom/mod/loader/ModLoader$1;->val$current:Ljava/lang/String;

    goto :goto_38

    :cond_36
    const-string v3, ""

    :goto_38
    invoke-virtual {v2, v3}, Landroid/widget/EditText;->setText(Ljava/lang/CharSequence;)V

    .line 50
    invoke-virtual {v2}, Landroid/widget/EditText;->getText()Landroid/text/Editable;

    move-result-object v3

    invoke-interface {v3}, Landroid/text/Editable;->length()I

    move-result v3

    invoke-virtual {v2, v3}, Landroid/widget/EditText;->setSelection(I)V

    .line 51
    const-string v3, "0 .. 4294967295"

    invoke-virtual {v2, v3}, Landroid/widget/EditText;->setHint(Ljava/lang/CharSequence;)V

    .line 52
    new-instance v3, Landroid/widget/LinearLayout$LayoutParams;

    const/4 v4, -0x1

    const/4 v5, -0x2

    invoke-direct {v3, v4, v5}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    .line 55
    const/16 v4, 0x8

    # invokes: Lcom/mod/loader/ModLoader;->dp(Landroid/content/Context;I)I
    invoke-static {v0, v4}, Lcom/mod/loader/ModLoader;->access$000(Landroid/content/Context;I)I

    move-result v0

    iput v0, v3, Landroid/widget/LinearLayout$LayoutParams;->topMargin:I

    .line 56
    invoke-virtual {v1, v2, v3}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 58
    new-instance v0, Landroid/app/AlertDialog$Builder;

    iget-object v3, p0, Lcom/mod/loader/ModLoader$1;->val$activity:Landroid/app/Activity;

    invoke-direct {v0, v3}, Landroid/app/AlertDialog$Builder;-><init>(Landroid/content/Context;)V

    const-string v3, "SEED_VALUE"

    .line 59
    invoke-virtual {v0, v3}, Landroid/app/AlertDialog$Builder;->setTitle(Ljava/lang/CharSequence;)Landroid/app/AlertDialog$Builder;

    move-result-object v0

    .line 60
    invoke-virtual {v0, v1}, Landroid/app/AlertDialog$Builder;->setView(Landroid/view/View;)Landroid/app/AlertDialog$Builder;

    move-result-object v0

    const-string v1, "OK"

    new-instance v3, Lcom/mod/loader/ModLoader$1$2;

    invoke-direct {v3, p0, v2}, Lcom/mod/loader/ModLoader$1$2;-><init>(Lcom/mod/loader/ModLoader$1;Landroid/widget/EditText;)V

    .line 61
    invoke-virtual {v0, v1, v3}, Landroid/app/AlertDialog$Builder;->setPositiveButton(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)Landroid/app/AlertDialog$Builder;

    move-result-object v0

    const-string v1, "RANDOM"

    new-instance v3, Lcom/mod/loader/ModLoader$1$1;

    invoke-direct {v3, p0}, Lcom/mod/loader/ModLoader$1$1;-><init>(Lcom/mod/loader/ModLoader$1;)V

    .line 67
    invoke-virtual {v0, v1, v3}, Landroid/app/AlertDialog$Builder;->setNeutralButton(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)Landroid/app/AlertDialog$Builder;

    move-result-object v0

    const-string v1, "CANCEL"

    .line 73
    const/4 v3, 0x0

    invoke-virtual {v0, v1, v3}, Landroid/app/AlertDialog$Builder;->setNegativeButton(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)Landroid/app/AlertDialog$Builder;

    move-result-object v0

    .line 74
    invoke-virtual {v0}, Landroid/app/AlertDialog$Builder;->create()Landroid/app/AlertDialog;

    move-result-object v0

    .line 75
    invoke-virtual {v0}, Landroid/app/AlertDialog;->show()V

    .line 76
    invoke-virtual {v2}, Landroid/widget/EditText;->requestFocus()Z
    :try_end_95
    .catchall {:try_start_0 .. :try_end_95} :catchall_96

    .line 78
    goto :goto_97

    .line 77
    :catchall_96
    move-exception v0

    .line 79
    :goto_97
    return-void
.end method
