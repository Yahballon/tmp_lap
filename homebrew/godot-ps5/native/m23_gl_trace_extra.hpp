// SPDX-License-Identifier: GPL-3.0-or-later
// M23: additional real Godot 4.7.2 GLAD function pointers whose calls
// can legitimately block on GPU resource residency or driver fences.
// Generated from a fixed, audited list. All wrappers call the original
// function exactly once, preserving args, return values and GL state.
// Included INSIDE the existing native launcher's anonymous namespace,
// AFTER ps5_now_ms(). Diagnostic only; not a claimed FPS fix.
struct M23GlStat {
    const char *name;
    std::atomic<unsigned long long> calls{0}, total_us{0}, max_us{0}, spikes_80ms{0};
};
M23GlStat m23_stats[]={
    {"glBindTexture"},
    {"glActiveTexture"},
    {"glBindVertexArray"},
    {"glGenVertexArrays"},
    {"glDeleteVertexArrays"},
    {"glGenBuffers"},
    {"glDeleteBuffers"},
    {"glGetSynciv"},
    {"glDeleteSync"},
    {"glGetProgramiv"},
    {"glGetShaderiv"},
    {"glGetUniformBlockIndex"},
    {"glTexParameteri"},
    {"glEnable"},
    {"glDisable"},
    {"glVertexAttribPointer"},
    {"glEnableVertexAttribArray"},
    {"glDisableVertexAttribArray"},
    {"glVertexAttribDivisor"},
    {"glBindBufferBase"},
    {"glBindBufferRange"},
    {"glBlendFuncSeparate"},
    {"glBlendEquationSeparate"},
    {"glViewport"},
    {"glScissor"},
    {"glFramebufferTexture2D"},
    {"glCheckFramebufferStatus"},
    {"glUniform1i"},
    {"glUniformMatrix4fv"},
    {"glBindSampler"},
    {"glGetAttribLocation"},
    {"glGetString"},
};
constexpr size_t m23_count=sizeof(m23_stats)/sizeof(m23_stats[0]);
unsigned m23_hooked=0;
double m23_window_begin=0;
void m23_sample(size_t idx,double millis) {
    auto us=static_cast<unsigned long long>(std::max(0.0,millis)*1000.0);
    M23GlStat &st=m23_stats[idx];
    st.calls.fetch_add(1,std::memory_order_relaxed);
    st.total_us.fetch_add(us,std::memory_order_relaxed);
    if(us>=80000)st.spikes_80ms.fetch_add(1,std::memory_order_relaxed);
    auto peak=st.max_us.load(std::memory_order_relaxed);
    while(peak<us && !st.max_us.compare_exchange_weak(peak,us,std::memory_order_relaxed,std::memory_order_relaxed)) {}
}
void m23_report(double now) {
    if(m23_window_begin==0) {
        m23_window_begin=now;
        std::printf("GODOT_PS5_M23_GL_PROFILER_READY hooked=%u names=%zu threshold_ms=80\n",m23_hooked,m23_count);
    }
    if(now-m23_window_begin<5000)return;
    unsigned long long calls=0,us=0,peak=0,spikes=0;
    for(size_t i=0;i<m23_count;++i) {
        auto &st=m23_stats[i];
        const auto c=st.calls.exchange(0,std::memory_order_relaxed);
        const auto t=st.total_us.exchange(0,std::memory_order_relaxed);
        const auto p=st.max_us.exchange(0,std::memory_order_relaxed);
        const auto n=st.spikes_80ms.exchange(0,std::memory_order_relaxed);
        calls+=c;us+=t;spikes+=n;peak=std::max(peak,p);
        if(c>0)std::printf("GODOT_PS5_M23_GL_5S name=%s calls=%llu total_ms=%.2f max_call_ms=%.2f spikes_80ms=%llu\n",st.name,c,t/1000.0,p/1000.0,n);
    }
    std::printf("GODOT_PS5_M23_GL_SUMMARY_5S hooked=%u calls=%llu total_ms=%.2f max_call_ms=%.2f spikes_80ms=%llu\n",m23_hooked,calls,us/1000.0,peak/1000.0,spikes);
    m23_window_begin=now;
}
PFNGLBINDTEXTUREPROC m23_original_glBindTexture=nullptr;
void m23_trace_glBindTexture(GLenum target, GLuint texture) {
    const double t0=ps5_now_ms();
    m23_original_glBindTexture(target,texture);
    m23_sample(0,ps5_now_ms()-t0);
}
PFNGLACTIVETEXTUREPROC m23_original_glActiveTexture=nullptr;
void m23_trace_glActiveTexture(GLenum texture) {
    const double t0=ps5_now_ms();
    m23_original_glActiveTexture(texture);
    m23_sample(1,ps5_now_ms()-t0);
}
PFNGLBINDVERTEXARRAYPROC m23_original_glBindVertexArray=nullptr;
void m23_trace_glBindVertexArray(GLuint array) {
    const double t0=ps5_now_ms();
    m23_original_glBindVertexArray(array);
    m23_sample(2,ps5_now_ms()-t0);
}
PFNGLGENVERTEXARRAYSPROC m23_original_glGenVertexArrays=nullptr;
void m23_trace_glGenVertexArrays(GLsizei n, GLuint *arrays) {
    const double t0=ps5_now_ms();
    m23_original_glGenVertexArrays(n,arrays);
    m23_sample(3,ps5_now_ms()-t0);
}
PFNGLDELETEVERTEXARRAYSPROC m23_original_glDeleteVertexArrays=nullptr;
void m23_trace_glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
    const double t0=ps5_now_ms();
    m23_original_glDeleteVertexArrays(n,arrays);
    m23_sample(4,ps5_now_ms()-t0);
}
PFNGLGENBUFFERSPROC m23_original_glGenBuffers=nullptr;
void m23_trace_glGenBuffers(GLsizei n, GLuint *buffers) {
    const double t0=ps5_now_ms();
    m23_original_glGenBuffers(n,buffers);
    m23_sample(5,ps5_now_ms()-t0);
}
PFNGLDELETEBUFFERSPROC m23_original_glDeleteBuffers=nullptr;
void m23_trace_glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    const double t0=ps5_now_ms();
    m23_original_glDeleteBuffers(n,buffers);
    m23_sample(6,ps5_now_ms()-t0);
}
PFNGLGETSYNCIVPROC m23_original_glGetSynciv=nullptr;
void m23_trace_glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) {
    const double t0=ps5_now_ms();
    m23_original_glGetSynciv(sync,pname,bufSize,length,values);
    m23_sample(7,ps5_now_ms()-t0);
}
PFNGLDELETESYNCPROC m23_original_glDeleteSync=nullptr;
void m23_trace_glDeleteSync(GLsync sync) {
    const double t0=ps5_now_ms();
    m23_original_glDeleteSync(sync);
    m23_sample(8,ps5_now_ms()-t0);
}
PFNGLGETPROGRAMIVPROC m23_original_glGetProgramiv=nullptr;
void m23_trace_glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    const double t0=ps5_now_ms();
    m23_original_glGetProgramiv(program,pname,params);
    m23_sample(9,ps5_now_ms()-t0);
}
PFNGLGETSHADERIVPROC m23_original_glGetShaderiv=nullptr;
void m23_trace_glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    const double t0=ps5_now_ms();
    m23_original_glGetShaderiv(shader,pname,params);
    m23_sample(10,ps5_now_ms()-t0);
}
PFNGLGETUNIFORMBLOCKINDEXPROC m23_original_glGetUniformBlockIndex=nullptr;
GLuint m23_trace_glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) {
    const double t0=ps5_now_ms();
    auto value=m23_original_glGetUniformBlockIndex(program,uniformBlockName);
    m23_sample(11,ps5_now_ms()-t0);
    return value;
}
PFNGLTEXPARAMETERIPROC m23_original_glTexParameteri=nullptr;
void m23_trace_glTexParameteri(GLenum target, GLenum pname, GLint param) {
    const double t0=ps5_now_ms();
    m23_original_glTexParameteri(target,pname,param);
    m23_sample(12,ps5_now_ms()-t0);
}
PFNGLENABLEPROC m23_original_glEnable=nullptr;
void m23_trace_glEnable(GLenum cap) {
    const double t0=ps5_now_ms();
    m23_original_glEnable(cap);
    m23_sample(13,ps5_now_ms()-t0);
}
PFNGLDISABLEPROC m23_original_glDisable=nullptr;
void m23_trace_glDisable(GLenum cap) {
    const double t0=ps5_now_ms();
    m23_original_glDisable(cap);
    m23_sample(14,ps5_now_ms()-t0);
}
PFNGLVERTEXATTRIBPOINTERPROC m23_original_glVertexAttribPointer=nullptr;
void m23_trace_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    const double t0=ps5_now_ms();
    m23_original_glVertexAttribPointer(index,size,type,normalized,stride,pointer);
    m23_sample(15,ps5_now_ms()-t0);
}
PFNGLENABLEVERTEXATTRIBARRAYPROC m23_original_glEnableVertexAttribArray=nullptr;
void m23_trace_glEnableVertexAttribArray(GLuint index) {
    const double t0=ps5_now_ms();
    m23_original_glEnableVertexAttribArray(index);
    m23_sample(16,ps5_now_ms()-t0);
}
PFNGLDISABLEVERTEXATTRIBARRAYPROC m23_original_glDisableVertexAttribArray=nullptr;
void m23_trace_glDisableVertexAttribArray(GLuint index) {
    const double t0=ps5_now_ms();
    m23_original_glDisableVertexAttribArray(index);
    m23_sample(17,ps5_now_ms()-t0);
}
PFNGLVERTEXATTRIBDIVISORPROC m23_original_glVertexAttribDivisor=nullptr;
void m23_trace_glVertexAttribDivisor(GLuint index, GLuint divisor) {
    const double t0=ps5_now_ms();
    m23_original_glVertexAttribDivisor(index,divisor);
    m23_sample(18,ps5_now_ms()-t0);
}
PFNGLBINDBUFFERBASEPROC m23_original_glBindBufferBase=nullptr;
void m23_trace_glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    const double t0=ps5_now_ms();
    m23_original_glBindBufferBase(target,index,buffer);
    m23_sample(19,ps5_now_ms()-t0);
}
PFNGLBINDBUFFERRANGEPROC m23_original_glBindBufferRange=nullptr;
void m23_trace_glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    const double t0=ps5_now_ms();
    m23_original_glBindBufferRange(target,index,buffer,offset,size);
    m23_sample(20,ps5_now_ms()-t0);
}
PFNGLBLENDFUNCSEPARATEPROC m23_original_glBlendFuncSeparate=nullptr;
void m23_trace_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    const double t0=ps5_now_ms();
    m23_original_glBlendFuncSeparate(srcRGB,dstRGB,srcAlpha,dstAlpha);
    m23_sample(21,ps5_now_ms()-t0);
}
PFNGLBLENDEQUATIONSEPARATEPROC m23_original_glBlendEquationSeparate=nullptr;
void m23_trace_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    const double t0=ps5_now_ms();
    m23_original_glBlendEquationSeparate(modeRGB,modeAlpha);
    m23_sample(22,ps5_now_ms()-t0);
}
PFNGLVIEWPORTPROC m23_original_glViewport=nullptr;
void m23_trace_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    const double t0=ps5_now_ms();
    m23_original_glViewport(x,y,width,height);
    m23_sample(23,ps5_now_ms()-t0);
}
PFNGLSCISSORPROC m23_original_glScissor=nullptr;
void m23_trace_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    const double t0=ps5_now_ms();
    m23_original_glScissor(x,y,width,height);
    m23_sample(24,ps5_now_ms()-t0);
}
PFNGLFRAMEBUFFERTEXTURE2DPROC m23_original_glFramebufferTexture2D=nullptr;
void m23_trace_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    const double t0=ps5_now_ms();
    m23_original_glFramebufferTexture2D(target,attachment,textarget,texture,level);
    m23_sample(25,ps5_now_ms()-t0);
}
PFNGLCHECKFRAMEBUFFERSTATUSPROC m23_original_glCheckFramebufferStatus=nullptr;
GLenum m23_trace_glCheckFramebufferStatus(GLenum target) {
    const double t0=ps5_now_ms();
    auto value=m23_original_glCheckFramebufferStatus(target);
    m23_sample(26,ps5_now_ms()-t0);
    return value;
}
PFNGLUNIFORM1IPROC m23_original_glUniform1i=nullptr;
void m23_trace_glUniform1i(GLint location, GLint v0) {
    const double t0=ps5_now_ms();
    m23_original_glUniform1i(location,v0);
    m23_sample(27,ps5_now_ms()-t0);
}
PFNGLUNIFORMMATRIX4FVPROC m23_original_glUniformMatrix4fv=nullptr;
void m23_trace_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    const double t0=ps5_now_ms();
    m23_original_glUniformMatrix4fv(location,count,transpose,value);
    m23_sample(28,ps5_now_ms()-t0);
}
PFNGLBINDSAMPLERPROC m23_original_glBindSampler=nullptr;
void m23_trace_glBindSampler(GLuint unit, GLuint sampler) {
    const double t0=ps5_now_ms();
    m23_original_glBindSampler(unit,sampler);
    m23_sample(29,ps5_now_ms()-t0);
}
PFNGLGETATTRIBLOCATIONPROC m23_original_glGetAttribLocation=nullptr;
GLint m23_trace_glGetAttribLocation(GLuint program, const GLchar *name) {
    const double t0=ps5_now_ms();
    auto value=m23_original_glGetAttribLocation(program,name);
    m23_sample(30,ps5_now_ms()-t0);
    return value;
}
PFNGLGETSTRINGPROC m23_original_glGetString=nullptr;
const GLubyte * m23_trace_glGetString(GLenum name) {
    const double t0=ps5_now_ms();
    auto value=m23_original_glGetString(name);
    m23_sample(31,ps5_now_ms()-t0);
    return value;
}
void *m23_resolve(const char *name,void *original) {
    if(std::strcmp(name,"glBindTexture")==0) {
        m23_original_glBindTexture=reinterpret_cast<PFNGLBINDTEXTUREPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glBindTexture);
    }
    if(std::strcmp(name,"glActiveTexture")==0) {
        m23_original_glActiveTexture=reinterpret_cast<PFNGLACTIVETEXTUREPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glActiveTexture);
    }
    if(std::strcmp(name,"glBindVertexArray")==0) {
        m23_original_glBindVertexArray=reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glBindVertexArray);
    }
    if(std::strcmp(name,"glGenVertexArrays")==0) {
        m23_original_glGenVertexArrays=reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGenVertexArrays);
    }
    if(std::strcmp(name,"glDeleteVertexArrays")==0) {
        m23_original_glDeleteVertexArrays=reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glDeleteVertexArrays);
    }
    if(std::strcmp(name,"glGenBuffers")==0) {
        m23_original_glGenBuffers=reinterpret_cast<PFNGLGENBUFFERSPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGenBuffers);
    }
    if(std::strcmp(name,"glDeleteBuffers")==0) {
        m23_original_glDeleteBuffers=reinterpret_cast<PFNGLDELETEBUFFERSPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glDeleteBuffers);
    }
    if(std::strcmp(name,"glGetSynciv")==0) {
        m23_original_glGetSynciv=reinterpret_cast<PFNGLGETSYNCIVPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGetSynciv);
    }
    if(std::strcmp(name,"glDeleteSync")==0) {
        m23_original_glDeleteSync=reinterpret_cast<PFNGLDELETESYNCPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glDeleteSync);
    }
    if(std::strcmp(name,"glGetProgramiv")==0) {
        m23_original_glGetProgramiv=reinterpret_cast<PFNGLGETPROGRAMIVPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGetProgramiv);
    }
    if(std::strcmp(name,"glGetShaderiv")==0) {
        m23_original_glGetShaderiv=reinterpret_cast<PFNGLGETSHADERIVPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGetShaderiv);
    }
    if(std::strcmp(name,"glGetUniformBlockIndex")==0) {
        m23_original_glGetUniformBlockIndex=reinterpret_cast<PFNGLGETUNIFORMBLOCKINDEXPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGetUniformBlockIndex);
    }
    if(std::strcmp(name,"glTexParameteri")==0) {
        m23_original_glTexParameteri=reinterpret_cast<PFNGLTEXPARAMETERIPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glTexParameteri);
    }
    if(std::strcmp(name,"glEnable")==0) {
        m23_original_glEnable=reinterpret_cast<PFNGLENABLEPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glEnable);
    }
    if(std::strcmp(name,"glDisable")==0) {
        m23_original_glDisable=reinterpret_cast<PFNGLDISABLEPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glDisable);
    }
    if(std::strcmp(name,"glVertexAttribPointer")==0) {
        m23_original_glVertexAttribPointer=reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glVertexAttribPointer);
    }
    if(std::strcmp(name,"glEnableVertexAttribArray")==0) {
        m23_original_glEnableVertexAttribArray=reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glEnableVertexAttribArray);
    }
    if(std::strcmp(name,"glDisableVertexAttribArray")==0) {
        m23_original_glDisableVertexAttribArray=reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glDisableVertexAttribArray);
    }
    if(std::strcmp(name,"glVertexAttribDivisor")==0) {
        m23_original_glVertexAttribDivisor=reinterpret_cast<PFNGLVERTEXATTRIBDIVISORPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glVertexAttribDivisor);
    }
    if(std::strcmp(name,"glBindBufferBase")==0) {
        m23_original_glBindBufferBase=reinterpret_cast<PFNGLBINDBUFFERBASEPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glBindBufferBase);
    }
    if(std::strcmp(name,"glBindBufferRange")==0) {
        m23_original_glBindBufferRange=reinterpret_cast<PFNGLBINDBUFFERRANGEPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glBindBufferRange);
    }
    if(std::strcmp(name,"glBlendFuncSeparate")==0) {
        m23_original_glBlendFuncSeparate=reinterpret_cast<PFNGLBLENDFUNCSEPARATEPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glBlendFuncSeparate);
    }
    if(std::strcmp(name,"glBlendEquationSeparate")==0) {
        m23_original_glBlendEquationSeparate=reinterpret_cast<PFNGLBLENDEQUATIONSEPARATEPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glBlendEquationSeparate);
    }
    if(std::strcmp(name,"glViewport")==0) {
        m23_original_glViewport=reinterpret_cast<PFNGLVIEWPORTPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glViewport);
    }
    if(std::strcmp(name,"glScissor")==0) {
        m23_original_glScissor=reinterpret_cast<PFNGLSCISSORPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glScissor);
    }
    if(std::strcmp(name,"glFramebufferTexture2D")==0) {
        m23_original_glFramebufferTexture2D=reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glFramebufferTexture2D);
    }
    if(std::strcmp(name,"glCheckFramebufferStatus")==0) {
        m23_original_glCheckFramebufferStatus=reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glCheckFramebufferStatus);
    }
    if(std::strcmp(name,"glUniform1i")==0) {
        m23_original_glUniform1i=reinterpret_cast<PFNGLUNIFORM1IPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glUniform1i);
    }
    if(std::strcmp(name,"glUniformMatrix4fv")==0) {
        m23_original_glUniformMatrix4fv=reinterpret_cast<PFNGLUNIFORMMATRIX4FVPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glUniformMatrix4fv);
    }
    if(std::strcmp(name,"glBindSampler")==0) {
        m23_original_glBindSampler=reinterpret_cast<PFNGLBINDSAMPLERPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glBindSampler);
    }
    if(std::strcmp(name,"glGetAttribLocation")==0) {
        m23_original_glGetAttribLocation=reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGetAttribLocation);
    }
    if(std::strcmp(name,"glGetString")==0) {
        m23_original_glGetString=reinterpret_cast<PFNGLGETSTRINGPROC>(original);
        ++m23_hooked;
        return reinterpret_cast<void*>(&m23_trace_glGetString);
    }
    return original;
}
