// Studio v49 frame-major animation. Layout and Quaternion48S packing are
// described by the local Portal 2 SDK; older RLE uses its existing decoder.
#ifndef BONE_FRAME_DECODE_H
#define BONE_FRAME_DECODE_H
struct SourceFrameAnimationHeader { int constantsOffset, frameOffset, frameLength, unused[3]; };
static Quaternion ReadFrameQuaternion( const byte *&data, bool sorted )
{
    Quaternion result;
    if ( !sorted ) { Quaternion48 packed; V_memcpy(&packed,data,6); result=(Quaternion)packed; }
    else
    {
        unsigned short v[3]; V_memcpy(v,data,sizeof(v));
        const int a = ((v[0]>>15)<<1) | (v[1]>>15);
        float *q = &result.x;
        for (int i=0;i<3;++i) q[(a+i)%4]=((int)(v[i]&32767)-16384)/23168.0f;
        q[(a+3)%4]=sqrtf(MAX(0.0f,1.0f-q[a]*q[a]-q[(a+1)%4]*q[(a+1)%4]-q[(a+2)%4]*q[(a+2)%4]));
        if(v[2]&32768) q[(a+3)%4]=-q[(a+3)%4];
    }
    data+=6; return result;
}
static Vector ReadFramePosition( const byte *&data, bool full )
{
    Vector result;
    if(full) { V_memcpy(&result,data,12); data+=12; }
    else { Vector48 packed; V_memcpy(&packed,data,6); result=(Vector)packed; data+=6; }
    return result;
}
static void ReadFramePose( const CStudioHdr *hdr, const studiohdr_t *animHdr,
    const virtualgroup_t *group, mstudioanimdesc_t &anim, int frame,
    Vector *pos, Quaternion *rot, int mask )
{
    float stall;
    const byte *base=(const byte *)anim.pAnim(&frame,stall);
    if(!base) return;
    const SourceFrameAnimationHeader *header=(const SourceFrameAnimationHeader *)base;
    const int bones=animHdr->numbones;
    if(bones<0 || bones>MAXSTUDIOBONES || header->constantsOffset<24+bones ||
        header->frameOffset<24+bones || header->frameLength<0) return;
    const byte *flags=base+24, *constants=base+header->constantsOffset;
    const byte *data=base+header->frameOffset+(size_t)frame*header->frameLength;
    for(int i=0;i<bones;++i)
    {
        const int j=group ? group->masterBone[i] : i;
        const mstudiobone_t *bone=animHdr->pBone(i);
        Quaternion q=bone->quat; Vector p=bone->pos;
        if(anim.flags&STUDIO_DELTA) { p.Init(); q.Init(0,0,0,1); }
        const byte f=flags[i];
        if(f&(0x02|0x40)) q=ReadFrameQuaternion(constants,(f&0x40)!=0);
        if(f&(0x01|0x20)) p=ReadFramePosition(constants,(f&0x20)!=0);
        if(f&(0x08|0x80)) q=ReadFrameQuaternion(data,(f&0x80)!=0);
        if(f&(0x04|0x10)) p=ReadFramePosition(data,(f&0x10)!=0);
        if(j>=0 && j<hdr->numbones() && (hdr->boneFlags(j)&mask)) { pos[j]=p; rot[j]=q; }
    }
}
static void CalcFrameAnimation( const CStudioHdr *hdr, const studiohdr_t *animHdr,
    const virtualgroup_t *group, mstudioanimdesc_t &anim, int frame, float fraction,
    Vector *pos, Quaternion *rot, int mask )
{
    ReadFramePose(hdr,animHdr,group,anim,frame,pos,rot,mask);
    if(fraction<=0 || frame+1>=anim.numframes) return;
    Vector nextPos[MAXSTUDIOBONES]; Quaternion nextRot[MAXSTUDIOBONES];
    for(int i=0;i<hdr->numbones();++i) { nextPos[i]=pos[i]; nextRot[i]=rot[i]; }
    ReadFramePose(hdr,animHdr,group,anim,frame+1,nextPos,nextRot,mask);
    for(int i=0;i<hdr->numbones();++i) if(hdr->boneFlags(i)&mask)
    { pos[i]=Lerp(fraction,pos[i],nextPos[i]); QuaternionSlerp(rot[i],nextRot[i],fraction,rot[i]); }
}
#endif
