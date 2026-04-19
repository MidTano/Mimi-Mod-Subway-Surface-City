.class public Lcom/mod/loader/ModLoader;
.super Ljava/lang/Object;
.source "ModLoader.java"


# static fields
.field private static volatile sHasNewSeedInput:Z

.field private static volatile sLastSeedInput:Ljava/lang/String;


# direct methods
.method static constructor <clinit>()V
    .registers 1

    .line 18
    const-string v0, "subway_mod"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V

    .line 21
    const-string v0, ""

    sput-object v0, Lcom/mod/loader/ModLoader;->sLastSeedInput:Ljava/lang/String;

    .line 22
    const/4 v0, 0x0

    sput-boolean v0, Lcom/mod/loader/ModLoader;->sHasNewSeedInput:Z

    return-void
.end method

.method public constructor <init>()V
    .registers 1

    .line 16
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method

.method static synthetic access$000(Landroid/content/Context;I)I
    .registers 2

    .line 16
    invoke-static {p0, p1}, Lcom/mod/loader/ModLoader;->dp(Landroid/content/Context;I)I

    move-result p0

    return p0
.end method

.method static synthetic access$102(Ljava/lang/String;)Ljava/lang/String;
    .registers 1

    .line 16
    sput-object p0, Lcom/mod/loader/ModLoader;->sLastSeedInput:Ljava/lang/String;

    return-object p0
.end method

.method static synthetic access$202(Z)Z
    .registers 1

    .line 16
    sput-boolean p0, Lcom/mod/loader/ModLoader;->sHasNewSeedInput:Z

    return p0
.end method

.method private static dp(Landroid/content/Context;I)I
    .registers 2

    .line 118
    invoke-virtual {p0}, Landroid/content/Context;->getResources()Landroid/content/res/Resources;

    move-result-object p0

    invoke-virtual {p0}, Landroid/content/res/Resources;->getDisplayMetrics()Landroid/util/DisplayMetrics;

    move-result-object p0

    iget p0, p0, Landroid/util/DisplayMetrics;->density:F

    .line 119
    int-to-float p1, p1

    mul-float p0, p0, p1

    const/high16 p1, 0x3f000000    # 0.5f

    add-float/2addr p0, p1

    float-to-int p0, p0

    return p0
.end method

.method private static findUnityActivity()Landroid/app/Activity;
    .registers 6

    .line 91
    const/4 v0, 0x0

    :try_start_1
    const-string v1, "com.unity3d.player.UnityPlayer"

    invoke-static {v1}, Ljava/lang/Class;->forName(Ljava/lang/String;)Ljava/lang/Class;

    move-result-object v1

    .line 92
    const-string v2, "currentActivity"

    invoke-virtual {v1, v2}, Ljava/lang/Class;->getField(Ljava/lang/String;)Ljava/lang/reflect/Field;

    move-result-object v1

    .line 93
    invoke-virtual {v1, v0}, Ljava/lang/reflect/Field;->get(Ljava/lang/Object;)Ljava/lang/Object;

    move-result-object v1

    .line 94
    instance-of v2, v1, Landroid/app/Activity;

    if-eqz v2, :cond_19

    check-cast v1, Landroid/app/Activity;
    :try_end_17
    .catchall {:try_start_1 .. :try_end_17} :catchall_18

    return-object v1

    .line 95
    :catchall_18
    move-exception v1

    :cond_19
    nop

    .line 97
    :try_start_1a
    const-string v1, "android.app.ActivityThread"

    invoke-static {v1}, Ljava/lang/Class;->forName(Ljava/lang/String;)Ljava/lang/Class;

    move-result-object v1

    .line 98
    const-string v2, "currentActivityThread"

    const/4 v3, 0x0

    new-array v4, v3, [Ljava/lang/Class;

    invoke-virtual {v1, v2, v4}, Ljava/lang/Class;->getMethod(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;

    move-result-object v2

    new-array v3, v3, [Ljava/lang/Object;

    invoke-virtual {v2, v0, v3}, Ljava/lang/reflect/Method;->invoke(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;

    move-result-object v2

    .line 99
    const-string v3, "mActivities"

    .line 100
    invoke-virtual {v1, v3}, Ljava/lang/Class;->getDeclaredField(Ljava/lang/String;)Ljava/lang/reflect/Field;

    move-result-object v1

    invoke-virtual {v1, v2}, Ljava/lang/reflect/Field;->get(Ljava/lang/Object;)Ljava/lang/Object;

    move-result-object v1

    check-cast v1, Ljava/util/Map;

    .line 101
    if-eqz v1, :cond_7e

    .line 102
    invoke-interface {v1}, Ljava/util/Map;->values()Ljava/util/Collection;

    move-result-object v1

    invoke-interface {v1}, Ljava/util/Collection;->iterator()Ljava/util/Iterator;

    move-result-object v1

    :goto_45
    invoke-interface {v1}, Ljava/util/Iterator;->hasNext()Z

    move-result v2

    if-eqz v2, :cond_7e

    invoke-interface {v1}, Ljava/util/Iterator;->next()Ljava/lang/Object;

    move-result-object v2

    .line 103
    invoke-virtual {v2}, Ljava/lang/Object;->getClass()Ljava/lang/Class;

    move-result-object v3

    const-string v4, "paused"

    invoke-virtual {v3, v4}, Ljava/lang/Class;->getDeclaredField(Ljava/lang/String;)Ljava/lang/reflect/Field;

    move-result-object v3

    .line 104
    const/4 v4, 0x1

    invoke-virtual {v3, v4}, Ljava/lang/reflect/Field;->setAccessible(Z)V

    .line 105
    invoke-virtual {v3, v2}, Ljava/lang/reflect/Field;->getBoolean(Ljava/lang/Object;)Z

    move-result v3

    .line 106
    if-eqz v3, :cond_64

    goto :goto_45

    .line 107
    :cond_64
    invoke-virtual {v2}, Ljava/lang/Object;->getClass()Ljava/lang/Class;

    move-result-object v3

    const-string v5, "activity"

    invoke-virtual {v3, v5}, Ljava/lang/Class;->getDeclaredField(Ljava/lang/String;)Ljava/lang/reflect/Field;

    move-result-object v3

    .line 108
    invoke-virtual {v3, v4}, Ljava/lang/reflect/Field;->setAccessible(Z)V

    .line 109
    invoke-virtual {v3, v2}, Ljava/lang/reflect/Field;->get(Ljava/lang/Object;)Ljava/lang/Object;

    move-result-object v2

    .line 110
    instance-of v3, v2, Landroid/app/Activity;

    if-eqz v3, :cond_7c

    check-cast v2, Landroid/app/Activity;
    :try_end_7b
    .catchall {:try_start_1a .. :try_end_7b} :catchall_7d

    return-object v2

    .line 111
    :cond_7c
    goto :goto_45

    .line 113
    :catchall_7d
    move-exception v1

    :cond_7e
    nop

    .line 114
    return-object v0
.end method

.method public static pollSeedInput()Ljava/lang/String;
    .registers 1

    .line 84
    sget-boolean v0, Lcom/mod/loader/ModLoader;->sHasNewSeedInput:Z

    if-nez v0, :cond_6

    const/4 v0, 0x0

    return-object v0

    .line 85
    :cond_6
    const/4 v0, 0x0

    sput-boolean v0, Lcom/mod/loader/ModLoader;->sHasNewSeedInput:Z

    .line 86
    sget-object v0, Lcom/mod/loader/ModLoader;->sLastSeedInput:Ljava/lang/String;

    return-object v0
.end method

.method public static promptSeedInput(Ljava/lang/String;)V
    .registers 3

    .line 29
    invoke-static {}, Lcom/mod/loader/ModLoader;->findUnityActivity()Landroid/app/Activity;

    move-result-object v0

    .line 30
    if-nez v0, :cond_7

    .line 31
    return-void

    .line 33
    :cond_7
    new-instance v1, Lcom/mod/loader/ModLoader$1;

    invoke-direct {v1, v0, p0}, Lcom/mod/loader/ModLoader$1;-><init>(Landroid/app/Activity;Ljava/lang/String;)V

    invoke-virtual {v0, v1}, Landroid/app/Activity;->runOnUiThread(Ljava/lang/Runnable;)V

    .line 81
    return-void
.end method


# virtual methods
.method public native initMod()V
.end method

.method public native isMenuVisible()Z
.end method

.method public native toggleMenu()V
.end method
