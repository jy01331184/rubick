//
// Created by tianyang on 2018/11/14.
//
#include "rubick.h"

jclass rubickClass;
jmethodID needHookMethod;

void initRubick(JNIEnv * env){
    rubickClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/rubick/Rubick")));

    needHookMethod = env->GetStaticMethodID(rubickClass,"needHook","()Z");
}