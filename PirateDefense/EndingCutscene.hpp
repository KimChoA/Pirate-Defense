#pragma once
#include <SFML/Graphics.hpp>
#include "SpriteAnimation.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace dw {
// Presentation only. No game, combat, network or save state is changed here.
// One deterministic timeline is shared by the game and the exported preview.
class EndingCutscene {
    sf::Texture atlas, closedChest, openChest;
    std::array<sf::Texture,5> crew;
    sf::Font font;
    static float sat(float x) { return std::clamp(x,0.f,1.f); }
    static float ease(float x) { x=sat(x);return x*x*(3-2*x); }
    static float mix(float a,float b,float x) { return a+(b-a)*x; }
    void rect(sf::RenderTarget& r,float x,float y,float w,float h,sf::Color c) {
        sf::RectangleShape q({w,h});q.setPosition({std::round(x),std::round(y)});q.setFillColor(c);r.draw(q);
    }
    void label(sf::RenderTarget& r,const std::string& s,float x,float y,unsigned size,sf::Color c={255,230,169},bool center=true) {
        sf::Text txt(font,sf::String::fromUtf8(s.begin(),s.end()),size);
        if(center){auto b=txt.getLocalBounds();txt.setOrigin({b.position.x+b.size.x/2,0});}
        txt.setPosition({x+1,y+1});txt.setFillColor({5,15,22,230});r.draw(txt);
        txt.setPosition({x,y});txt.setFillColor(c);r.draw(txt);
    }
    void panel(sf::RenderTarget& r,float x,float y,float w,float h) {
        rect(r,x-2,y-2,w+4,h+4,{38,27,29,240});rect(r,x,y,w,h,{169,123,61});
        rect(r,x+2,y+2,w-4,h-4,{24,42,49,247});rect(r,x+4,y+4,w-8,1,{90,107,99});
    }
    void sprite(sf::RenderTarget& r,const sf::Texture& tex,float x,float bottom,float w,float h,sf::Color tint=sf::Color::White) {
        sf::Sprite s(tex);auto z=tex.getSize();s.setOrigin({z.x*.5f,float(z.y)});
        s.setPosition({std::round(x),std::round(bottom)});s.setScale({w/z.x,h/z.y});s.setColor(tint);r.draw(s);
    }
    void background(sf::RenderTarget& r,int scene,float t) {
        auto z=atlas.getSize();int w=int(z.x)/2,h=int(z.y)/2;
        sf::Sprite s(atlas);s.setTextureRect(sf::IntRect({(scene%2)*w,(scene/2)*h},{w,h}));
        float pan=scene==1 ? 18*ease((t-8)/8):0;
        s.setPosition({-pan,0});s.setScale({(640.f+(scene==1?20:0))/w,360.f/h});r.draw(s);
        // Small pixel glints drift independently of the plate.
        if(scene==0 || scene==3) for(int i=0;i<26;++i) {
            float x=scene==0?310.f+std::fmod(i*29.f+t*7,205.f):185.f+std::fmod(i*29.f+t*5,345.f);
            float y=(scene==0?224.f:210.f)+(i%6)*5;
            auto a=std::uint8_t(35+35*(1+std::sin(t*2+i)));
            rect(r,x,y,3+(i%3)*2,1,{255,224,155,a});
        }
        if(scene==1)for(int i=0;i<20;++i) {
            float x=143+(i%4)*4-pan,y=132+std::fmod(i*11+t*30,110.f);
            rect(r,x,y,1,4,{190,248,238,100});
        }
    }
    void star(sf::RenderTarget& r,float x,float y,float a,float sz=2) {
        if(a<=0)return;auto alpha=std::uint8_t(255*sat(a));
        rect(r,x-sz,y,sz*2+1,1,{255,206,66,alpha});rect(r,x,y-sz,1,sz*2+1,{255,239,166,alpha});
        rect(r,x,y,1,1,{255,255,232,alpha});
    }
    void gem(sf::RenderTarget& r,float x,float y,float s,sf::Color c) {
        sf::ConvexShape q(4);q.setPoint(0,{0,-s});q.setPoint(1,{s,0});q.setPoint(2,{0,s});q.setPoint(3,{-s,0});
        q.setPosition({std::round(x),std::round(y)});q.setFillColor(c);q.setOutlineThickness(1);q.setOutlineColor({38,36,61});r.draw(q);
        rect(r,x-1,y-s+2,2,s-2,{235,255,255});
    }
    void coin(sf::RenderTarget& r,float x,float y,float t) {
        float w=2+3*std::abs(std::cos(t*3));rect(r,x-w,y-5,w*2,10,{127,64,26});
        rect(r,x-w+1,y-4,w*2-2,8,{255,207,55});rect(r,x,y-3,1,6,{255,248,158});
    }
    void actor(sf::RenderTarget& r,int id,float x,float feet,float scale,float t,bool walk,bool cheer=false,int role=0) {
        const auto& tex=crew[id];auto z=tex.getSize();float w=z.x*scale,h=z.y*scale;
        float hop=cheer?std::max(0.f,std::sin(t*5+id*1.1f))*7:0;
        sf::CircleShape shadow(1,24);shadow.setScale({w*.33f,3});shadow.setPosition({x-w*.33f,feet-3});shadow.setFillColor({7,16,22,100});r.draw(shadow);
        visual::Pose p;p.motion=walk?visual::Motion::Walk:visual::Motion::Idle;
        p.phase=std::fmod(t*(walk?2.5f:.6f)+id*.17f,1.f);
        std::vector<sf::Vertex> verts;verts.reserve(z.y*8*6);
        auto vertex=[&](float u,float v){auto d=visual::deform(u,v,p,visual::Rig::Humanoid);
            if(cheer && v>.32f && v<.7f && (u<.24f || u>.76f))d.y-=3.f*(1.f-v);
            return sf::Vertex{{std::round(x+(u-.5f)*w+d.x*scale),std::round(feet-h+v*h+d.y*scale-hop)},sf::Color::White,{u*z.x,v*z.y}};};
        for(unsigned row=0;row<z.y;++row)for(int col=0;col<8;++col){
            float u=col/8.f,v=float(row)/z.y,uu=(col+1)/8.f,vv=float(row+1)/z.y;
            auto a=vertex(u,v),b=vertex(uu,v),c=vertex(uu,vv),d=vertex(u,vv);
            verts.insert(verts.end(),{a,b,c,a,c,d});}
        sf::RenderStates state;state.texture=&tex;r.draw(verts.data(),verts.size(),sf::PrimitiveType::Triangles,state);
        float handY=feet-h*.46f-hop;
        if(role==1){ // unfolding map
            rect(r,x+4,handY,22,15,{82,49,29});rect(r,x+5,handY+1,20,13,{241,213,143});
            rect(r,x+14,handY+2,1,10,{182,142,85});rect(r,x+8,handY+6,8,1,{99,117,72});
            rect(r,x+20,handY+7,3,3,{186,62,38});
        } else if(role==2){ // clear pointing gesture
            rect(r,x+w*.27f,handY-2,14,5,{82,42,27});rect(r,x+w*.27f+2,handY-3,14,4,{255,204,145});
        } else if(role==3){
            sf::CircleShape glass(5,12);glass.setPosition({x+w*.25f,handY-4});glass.setFillColor({123,224,234,150});glass.setOutlineThickness(2);glass.setOutlineColor({92,55,30});r.draw(glass);
            rect(r,x+w*.25f+8,handY+6,3,8,{178,115,49});
        } else if(role==4)coin(r,x+w*.38f,handY-17,t);
        else if(role==5)gem(r,x-w*.35f,handY-10,6,{56,213,255});
    }
    void treasure(sf::RenderTarget& r,float x,float bottom,float opening,float t,float scale=1) {
        float w=94*scale,h=68*scale;
        if(opening>0){
            // Animated light rays emanate from the open chest, never a static overlay.
            for(int i=0;i<9;++i){float ang=-2.8f+i*.31f+std::sin(t*.55f)*.12f;
                float reach=(70+15*std::sin(t*1.5f+i))*scale*opening;
                sf::ConvexShape ray(3);ray.setPoint(0,{x,bottom-30*scale});
                ray.setPoint(1,{x+std::cos(ang-.06f)*reach,bottom-30*scale+std::sin(ang-.06f)*reach});
                ray.setPoint(2,{x+std::cos(ang+.06f)*reach,bottom-30*scale+std::sin(ang+.06f)*reach});ray.setFillColor({255,204,65,std::uint8_t(32*opening)});r.draw(ray);
            }
        }
        if(opening<1)sprite(r,closedChest,x+std::sin(t*42)*(opening>0?1:0),bottom,w,h,{255,255,255,std::uint8_t(255*(1-opening))});
        if(opening>0)sprite(r,openChest,x,bottom,w,h,{255,255,255,std::uint8_t(255*opening)});
        if(opening>.6f){
            for(int i=0;i<13;++i){float xx=x-40*scale+i*6*scale,yy=bottom-5+(i%3)*3;coin(r,xx,yy,t+i);}
            gem(r,x+19*scale,bottom-28*scale,5*scale,{66,235,223});gem(r,x-14*scale,bottom-31*scale,6*scale,{247,70,102});
            // Gold crown and pearl strand in the chest.
            rect(r,x-2,bottom-32*scale,15,5,{255,207,55});for(int j=0;j<3;++j)rect(r,x-2+j*6,bottom-37*scale,3,7,{255,229,105});
            for(int j=0;j<7;++j)rect(r,x+22+j*2,bottom-23*scale+j*2,2,2,{255,247,213});
            for(int i=0;i<48;++i){float life=std::fmod(t*.30f+i*.618f,1.f);float xx=x+std::sin(i*13.1f+life*.8f)*(18+life*72)*scale;
                float yy=bottom-20*scale-life*104*scale;star(r,xx,yy,std::sin(life*3.14159f)*opening,(i%4==0)?3:1);}
        }
    }
public:
    static constexpr float FinaleTime=27.f;
    EndingCutscene(){
        const std::string p="assets/images/pixel/";
        auto load=[](sf::Texture& t,const std::string& path){if(!t.loadFromFile(path))throw std::runtime_error("Ending asset missing: "+path);t.setSmooth(false);};
        load(atlas,p+"ending/cutscene_atlas.png");load(closedChest,p+"ending/treasure_chest_closed.png");load(openChest,p+"ending/treasure_chest_open.png");
        for(int i=0;i<5;++i)load(crew[i],p+"players/player"+std::to_string(i+1)+".png");
        if(!font.openFromFile("C:/Windows/Fonts/malgun.ttf"))throw std::runtime_error("Ending Korean font unavailable");
    }
    void draw(sf::RenderTarget& r,float t,bool client=false,bool preview=false){
        t=std::max(0.f,t);int scene=t<8?0:t<16?1:t<27?2:3;background(r,scene,t);
        if(scene==0){
            label(r,"보물섬",590,243,10,{255,235,165});
            for(int i=4;i>=0;--i){float u=sat((t-i*.55f)/4.8f);float feet=u<.45f?mix(203,308,u/.45f):mix(308,320,(u-.45f)/.55f);
                float x=u<.45f?mix(164,317,u/.45f):mix(317,630,(u-.45f)/.55f);
                actor(r,i,x,feet,1.45f,t,true);}
        }else if(scene==1){
            float local=t-8;for(int i=0;i<5;++i){float x=-40+local*68-i*51;actor(r,i,x,326+(i%2)*2,1.5f,t,true,false,i==0?2:i==1?1:i==3?3:0);}
        }else if(scene==2){
            float local=t-16,opening=ease((t-20)/1.5f);bool cheering=t>23;
            const float endX[]={259,315,444,487,209};const float endY[]={302,259,260,307,318};
            for(int i=0;i<5;++i)if(i==1||i==2){float u=ease((local-i*.12f)/3.8f);actor(r,i,mix(-20-i*43.f,endX[i],u),endY[i],1.6f,t,u<1,cheering,cheering&&i==2?4:0);}
            treasure(r,376,309,opening,t,1.15f);
            for(int i=0;i<5;++i)if(i!=1&&i!=2){float u=ease((local-i*.12f)/3.8f);actor(r,i,mix(-20-i*43.f,endX[i],u),endY[i],1.6f,t,u<1,cheering,cheering&&i==3?5:0);}
            if(t>19&&t<20.5f){label(r,"!",290,212,20);label(r,"!",454,202,20);}
            if(t>21.05f&&t<21.65f)rect(r,0,0,640,360,{255,235,157,std::uint8_t(100*(1-std::abs(t-21.35f)/.3f))});
        }else{
            // Same five crew members, arranged around the chest as in the reference.
            actor(r,1,270,245,1.85f,t,false,true);
            actor(r,2,379,245,1.85f,t,false,true,4);
            treasure(r,326,292,1,t,1.28f);
            actor(r,0,216,290,1.85f,t,false,true);
            actor(r,3,438,290,1.85f,t,false,true,5);
            actor(r,4,273,303,1.65f,t,false,true);
            for(int i=0;i<28;++i){float life=std::fmod(i*.371f+t*.055f,1.f);star(r,75+std::fmod(i*83.7f+t*3,490.f),295-life*220,std::sin(life*3.14159f)*.8f,1);}
        }
        if(scene<3){
            panel(r,178,13,284,38);
            const char* titles[]={"01  ·  보물섬에 상륙!","02  ·  지도 너머의 비밀","03  ·  잠들어 있던 보물"};
            label(r,titles[scene],320,22,14);
            panel(r,139,333,362,22);
            std::string caption=scene==0?"배에서 내려, 새로운 모험 속으로!":scene==1?"보물이 어디에 있을까?  ·  모두 함께 오른쪽으로!":t<20?"찾았다!  모두 이쪽으로 모여!":t<23?"오래된 보물상자가 열립니다…":"금화와 보석이 가득해!  우리가 해냈어!";
            label(r,caption,320,337,10);
        }else{
            panel(r,96,12,448,47);
            label(r,"최종 승리 · 보물섬에 도착했습니다",320,20,18);
            label(r,"바다 3개 + 섬 3개 스테이지 완료",320,44,9,{216,226,211});
            panel(r,96,306,448,43);
            label(r,"함께 항해해 주셔서 감사합니다.",320,311,11);
            label(r,preview?"Enter 엔딩 다시 보기  /  Esc 미리보기 닫기":client?"방장의 재시작을 기다리는 중  /  Esc 메인 메뉴":"Enter 처음부터 다시 항해  /  Esc 메인 메뉴",320,332,10,sf::Color::White);
        }
        // Fade through a brief ink-dark frame at every scene boundary.
        float fade=sat((.7f-t)/.7f);
        for(float cut:{8.f,16.f,27.f})fade=std::max(fade,sat(1-std::abs(t-cut)/.55f));
        if(fade>0)rect(r,0,0,640,360,{8,17,25,std::uint8_t(255*fade)});
    }
};
}
