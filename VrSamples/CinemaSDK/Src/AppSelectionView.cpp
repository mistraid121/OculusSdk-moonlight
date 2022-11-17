/************************************************************************************

Filename    :   AppSelectionView.cpp
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <unistd.h>
#include "AppSelectionView.h"
#include "CinemaApp.h"
#include "GUI/VRMenuMgr.h"
#include "GUI/GuiSys.h"
#include "PcCategoryComponent.h"
#include "MoviePosterComponent.h"
#include "MovieSelectionComponent.h"
#include "CarouselSwipeHintComponent.h"
#include "PackageFiles.h"
#include "CinemaStrings.h"
#include "Native.h"

using namespace OVRFW;
using OVR::Vector2f;
using OVR::Vector3f;
using OVR::Vector4f;
using OVR::Matrix4f;
using OVR::Quatf;
using OVR::Bounds3f;

namespace OculusCinema {

static const int PosterWidth = 228;
static const int PosterHeight = 344;

static const Vector3f PosterScale(4.4859375f * 0.98f);

static const double TimerTotalTime = 10;

static const int NumSwipeTrails = 3;
static const char* Guuid;

//=======================================================================================

    AppSelectionView::AppSelectionView(CinemaApp &cinema) :
            SelectionView("AppSelectionView"),
            Cinema(cinema),
            SelectionTexture(),
            Is3DIconTexture(),
            ShadowTexture(), BorderTexture(),
            SwipeIconLeftTexture(),
            SwipeIconRightTexture(),
            ResumeIconTexture(),
            ErrorIconTexture(),
            SDCardTexture(),
            CloseIconTexture(),
            SettingsIconTexture(),
            Menu(NULL),
            CenterRoot(NULL),
            ErrorMessage(NULL),
            SDCardMessage(NULL),
            PlainErrorMessage(NULL),
            ErrorMessageClicked(false),
            MovieRoot(NULL),
            CategoryRoot(NULL),
            TitleRoot(NULL),
            MovieTitle(NULL),
            SelectionFrame(NULL),
            CenterPoster(NULL),
            CenterIndex(0),
            CenterPosition(),
            LeftSwipes(),
            RightSwipes(),
            ResumeIcon(NULL),
            CloseAppButton( NULL ),
            SettingsButton( NULL ),
            TimerIcon(NULL),
            TimerText(NULL),
            TimerStartTime(0),
            TimerValue(0),
            ShowTimer(false),
            MoveScreenLabel(NULL),
            MoveScreenAlpha(),
            SelectionFader(),
            MovieBrowser(NULL),
            MoviePanelPositions(),
            MoviePosterComponents(),
            Categories(),
            CurrentCategory(CATEGORY_LIMELIGHT),
            AppList(),
            AppIndex(0),
            LastAppDisplayed(NULL),
            RepositionScreen(false),
            HadSelection( false ),
            settingsMenu( NULL ),
            bgTintTexture(),
            newPCbg( Cinema.GetGuiSys() ),
            ButtonGaze( NULL ),
            ButtonTrackpad( NULL ),
            ButtonOff( NULL ),
            Button169( NULL ),
            Button43( NULL ),
            Button4k60( NULL ),
            Button4k30( NULL ),
            Button1080p60( NULL ),
            Button1080p30( NULL ),
            Button720p60( NULL ),
            Button720p30( NULL ),
            ButtonHostAudio( NULL ),
            ButtonSaveApp( NULL ),
            ButtonSaveDefault( NULL ),
            mouseMode( MOUSE_GAZE ),
            streamAspectRatio(DIECISEIS_NOVENOS),
            streamWidth( 1280 ),
            streamHeight( 720 ),
            streamFPS( 60 ),
            streamHostAudio( 0 ),
            settingsVersion(1.0f),
            defaultSettingsPath(""),
            appSettingsPath(""),
            defaultSettings( NULL ),
            appSettings( NULL )


    {
        // This is called at library load time, so the system is not initialized
        // properly yet.
    }

    AppSelectionView::~AppSelectionView() {
        DeletePointerArray(MovieBrowserItems);
    }

    void AppSelectionView::OneTimeInit(const char *launchIntent) {
        OVR_UNUSED(launchIntent);

        OVR_LOG("AppSelectionView::OneTimeInit");


        const double start = GetTimeInSeconds();

        CreateMenu(Cinema.GetGuiSys());

//        SetCategory(CATEGORY_LIMELIGHT);


        OVR_LOG(
                "AppSelectionView::OneTimeInit %3.1f seconds", vrapi_GetTimeInSeconds() - start);

    }

    void AppSelectionView::OneTimeShutdown() {
        OVR_LOG("AppSelectionView::OneTimeShutdown");
    }

    void AppSelectionView::OnOpen(const double currTimeInSeconds ) {
        SetCategory(CATEGORY_LIMELIGHT);
        OVR_LOG("OnOpen");
        CurViewState = VIEWSTATE_OPEN;

        LastAppDisplayed = NULL;
        HadSelection = false;

        if (Cinema.InLobby) {
            Cinema.SceneMgr.SetSceneModel(*Cinema.ModelMgr.BoxOffice);

            Vector3f size(PosterWidth * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x,
                          PosterHeight * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.y, 0.0f);

            Cinema.SceneMgr.SceneScreenBounds = Bounds3f(size * -0.5f, size * 0.5f);
            Cinema.SceneMgr.SceneScreenBounds.Translate(Vector3f(0.00f, 1.76f, -7.25f));
        }

        Cinema.SceneMgr.LightsOn(1.5f,currTimeInSeconds);

        const double now = vrapi_GetTimeInSeconds();
        SelectionFader.Set(now, 0, now + 0.1, 1.0f);

        Menu->SetFlags(VRMENU_FLAG_SHORT_PRESS_HANDLED_BY_APP);

        if (Cinema.InLobby) {
            CategoryRoot->SetVisible(true);
        } else {
            CategoryRoot->SetVisible(false);


            ResumeIcon->SetVisible(false);
            TimerIcon->SetVisible(false);
            CenterRoot->SetVisible(true);

            MoveScreenLabel->SetVisible(false);


            MovieBrowser->SetSelectionIndex(AppIndex);


            RepositionScreen = false;
            MoveScreenAlpha.Set(0, 0, 0, 0.0f);

            UpdateMenuPosition();
            Cinema.SceneMgr.ClearGazeCursorGhosts();
            Menu->Open();

            MoviePosterComponent::ShowShadows = Cinema.InLobby;

            CarouselSwipeHintComponent::ShowSwipeHints = true;

            const PcDef *selectedPC = Cinema.GetCurrentPc();
            std::string uuid = selectedPC->UUID;
            Guuid = uuid.c_str();
            Native::PairState ps = Native::GetPairState(Guuid);
            if (ps == Native::PAIRED) {
                OVR_LOG("Paired");
                Native::InitAppSelector( Guuid);
            } else {
                OVR_LOG("Not Paired!");
                Native::Pair(Guuid);
            }

        }
    }

    void AppSelectionView::Frame(const ovrApplFrameIn &vrFrame) {
            // We want 4x MSAA in the lobby
            /*
            ovrEyeBufferParms eyeBufferParms = Cinema.app->GetEyeBufferParms();
            eyeBufferParms.multisamples = 4;
            Cinema.app->SetEyeBufferParms(eyeBufferParms);*/

#if 0
            if ( !Cinema.InLobby && Cinema.SceneMgr.ChangeSeats( vrFrame ) )
            {
                UpdateMenuPosition();
            }
#endif

            //if (vrFrame.Input.buttonPressed & BUTTON_B) {
                /*if (Cinema.InLobby) {
                    Cinema.app->ShowSystemUI(VRAPI_SYS_UI_CONFIRM_QUIT_MENU);
                } else {
                    Cinema.GetGuiSys().CloseMenu(Menu->GetVRMenu(), false);
                }*/
            //}

            // check if they closed the menu with the back button
            if (!Cinema.InLobby && Menu->GetVRMenu()->IsClosedOrClosing() &&
                !Menu->GetVRMenu()->IsOpenOrOpening()) {
                // if we finished the movie or have an error, don't resume it, go back to the lobby
                if (ErrorShown()) {
                    OVR_LOG("Error closed.  Return to lobby.");
                    ClearError();
                    Cinema.PcSelection(true);
                } else if (Cinema.IsMovieFinished()) {
                    OVR_LOG("Movie finished.  Return to lobby.");
                    Cinema.PcSelection(true);
                } else {
                    OVR_LOG("Resume movie.");
                    Cinema.PlayOrResumeOrRestartApp();
                }
            }

            if (!Cinema.InLobby && ErrorShown()) {
                CarouselSwipeHintComponent::ShowSwipeHints = true;
                if (vrFrame.AllTouches | ovrTouch_A) {
                    Cinema.GetGuiSys().GetSoundEffectPlayer().Play("touch_down");
                } else if (vrFrame.LastFrameAllButtons | ovrTouch_A) {
                    Cinema.GetGuiSys().GetSoundEffectPlayer().Play("touch_up");
                    ErrorMessageClicked = true;
                } else if (ErrorMessageClicked &&
                        (vrFrame.AllTouches | ovrTouch_A) == 0) {
                    Menu->Close();
                }
            }

            if (Cinema.SceneMgr.FreeScreenActive && !ErrorShown()) {
                if (!RepositionScreen && !SelectionFrame->GetMenuObject()->IsHilighted()) {
                    // outside of screen, so show reposition message
                    const double now = vrapi_GetTimeInSeconds();
                    float alpha = static_cast<float>( MoveScreenAlpha.Value(now));
                    if (alpha > 0.0f) {
                        MoveScreenLabel->SetVisible(true);
                        MoveScreenLabel->SetTextColor(Vector4f(alpha));
                    }

                    if (vrFrame.AllTouches | ovrTouch_A) {
                        // disable hit detection on selection frame
                        SelectionFrame->GetMenuObject()->AddFlags(VRMENUOBJECT_DONT_HIT_ALL);
                        RepositionScreen = true;
                    }
                } else {
                    // onscreen, so hide message
                    const double now = vrapi_GetTimeInSeconds();
                    MoveScreenAlpha.Set(now, -1.0f, now + 1.0f, 1.0f);
                    MoveScreenLabel->SetVisible(false);
                }

                const Matrix4f invViewMatrix(Cinema.SceneMgr.Scene.GetCenterEyeTransform());
                const Vector3f viewPos(Cinema.SceneMgr.Scene.GetCenterEyePosition());
                const Vector3f viewFwd(Cinema.SceneMgr.Scene.GetCenterEyeForward());

                // spawn directly in front
                Quatf rotation(-viewFwd, 0.0f);
                Quatf viewRot(invViewMatrix);
                Quatf fullRotation = rotation * viewRot;

                const float menuDistance = 1.45f;
                Vector3f position(viewPos + viewFwd * menuDistance);

                MoveScreenLabel->SetLocalPose(fullRotation, position);
            }

            // while we're holding down the button or touchpad, reposition screen
            if (RepositionScreen) {
                if (vrFrame.AllTouches | ovrTouch_A) {
                    Cinema.SceneMgr.PutScreenInFront();
                    Quatf orientation = Quatf(Cinema.SceneMgr.FreeScreenPose);
                    CenterRoot->GetMenuObject()->SetLocalRotation(orientation);
                    CenterRoot->GetMenuObject()->SetLocalPosition(
                            Cinema.SceneMgr.FreeScreenPose.Transform(
                                    Vector3f(0.0f, -1.76f * 0.55f, 0.0f)));

                } else {
                    RepositionScreen = false;
                }
            } else {
                // reenable hit detection on selection frame.
                // note: we do this on the frame following the frame we disabled RepositionScreen on
                // so that the selection object doesn't get the touch up.
                SelectionFrame->GetMenuObject()->RemoveFlags(VRMENUOBJECT_DONT_HIT_ALL);
            }

            UpdateAppTitle();
            UpdateSelectionFrame(vrFrame);

            Cinema.SceneMgr.Frame(vrFrame);
        }

    const AppDef *AppSelectionView::GetSelectedApp() const {
        int selectedItem = MovieBrowser->GetSelection();
        if ((selectedItem >= 0) && (selectedItem < static_cast<int>(AppList.size()))) {
            return AppList[selectedItem];
        }

        return NULL;
    }

    void AppSelectionView::UpdateAppTitle() {
        const AppDef * currentMovie = GetSelectedApp();
        if (LastAppDisplayed != currentMovie) {
            if (currentMovie != NULL) {
                MovieTitle->SetText(currentMovie->Name.c_str());
            } else {
                MovieTitle->SetText("");
            }

            LastAppDisplayed = currentMovie;
        }
    }

    void AppSelectionView::OnClose() {
        OVR_LOG("OnClose");
        ShowTimer = false;
        CurViewState = VIEWSTATE_CLOSED;
        CenterRoot->SetVisible(false);
        Menu->Close();
        Cinema.SceneMgr.ClearMovie();
    }

    void AppSelectionView::PairSuccess() {
        //const PcDef *selectedPC = Cinema.GetCurrentPc();
        //String uuid = selectedPC->UUID;
        //Native::PairState ps = Native::GetPairState(Cinema.app, Guuid);
        Cinema.AppSelection(false);
    }
    void AppCloseAppButtonCallback( UIButton *button, void *object )
    {
        ( ( AppSelectionView * )object )->CloseAppButtonPressed();
    }

    void SettingsButtonCallback( UIButton *button, void *object )
    {
        ( ( AppSelectionView * )object )->SettingsButtonPressed();
    }

    void SettingsCallback( UIButton *button, void *object )
    {
        ( ( AppSelectionView * )object )->SettingsPressed(button);
        button->SetSelected(true);
    }

    bool SettingsActiveCallback( UIButton *button, void *object )
    {
        return ( ( AppSelectionView * )object )->SettingsIsActive(button);
    }



    void AppSelectionView::TextButtonHelper(UIButton* button, float scale, int w, int h)
    {
        button->SetLocalScale( Vector3f( scale ) );
        //button->SetFontScale( 1.0f );
        button->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
        //button->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
        button->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, w, h );
    }

    void AppSelectionView::CreateMenu(OvrGuiSys &guiSys) {
        OVR_UNUSED(guiSys);

        const Quatf forward(Vector3f(0.0f, 1.0f, 0.0f), 0.0f);

        // ==============================================================================
        //
        // load textures
        //
        SelectionTexture.LoadTextureFromApplicationPackage("assets/selection.png");
        Is3DIconTexture.LoadTextureFromApplicationPackage("assets/3D_icon.png");
        ShadowTexture.LoadTextureFromApplicationPackage("assets/shadow.png");
        BorderTexture.LoadTextureFromApplicationPackage("assets/category_border.png");
        SwipeIconLeftTexture.LoadTextureFromApplicationPackage(
                "assets/SwipeSuggestionArrowLeft.png");
        SwipeIconRightTexture.LoadTextureFromApplicationPackage(
                "assets/SwipeSuggestionArrowRight.png");
        ResumeIconTexture.LoadTextureFromApplicationPackage("assets/resume.png");
        ErrorIconTexture.LoadTextureFromApplicationPackage("assets/error.png");
        SDCardTexture.LoadTextureFromApplicationPackage("assets/sdcard.png");
        CloseIconTexture.LoadTextureFromApplicationPackage( "assets/close.png" );
        SettingsIconTexture.LoadTextureFromApplicationPackage( "assets/settings.png" );

        bgTintTexture.LoadTextureFromApplicationPackage( "assets/backgroundTint.png" );

        ButtonTexture.LoadTextureFromApplicationPackage( "assets/button.png" );
        ButtonHoverTexture.LoadTextureFromApplicationPackage( "assets/button_hoover.png" );
        ButtonPressedTexture.LoadTextureFromApplicationPackage( "assets/button_pressed.png" );
        // ==============================================================================
        //
        // create menu
        //
        Menu = new UIMenu(Cinema.GetGuiSys());
        Menu->Create("MovieBrowser");

        CenterRoot = new UIContainer(Cinema.GetGuiSys());
        CenterRoot->AddToMenu(Menu);
        CenterRoot->SetLocalPose(forward, Vector3f(0.0f, 0.0f, 0.0f));

        MovieRoot = new UIContainer(Cinema.GetGuiSys());
        MovieRoot->AddToMenu(Menu, CenterRoot);
        MovieRoot->SetLocalPose(forward, Vector3f(0.0f, 0.0f, 0.0f));

        CategoryRoot = new UIContainer(Cinema.GetGuiSys());
        CategoryRoot->AddToMenu(Menu, CenterRoot);
        CategoryRoot->SetLocalPose(forward, Vector3f(0.0f, 0.0f, 0.0f));

        TitleRoot = new UIContainer(Cinema.GetGuiSys());
        TitleRoot->AddToMenu(Menu, CenterRoot);
        TitleRoot->SetLocalPose(forward, Vector3f(0.0f, 0.0f, 0.0f));

        // ==============================================================================
        //
        // error message
        //
        ErrorMessage = new UILabel(Cinema.GetGuiSys());
        ErrorMessage->AddToMenu(Menu, CenterRoot);
        ErrorMessage->SetLocalPose(forward, Vector3f(0.00f, 1.76f, -7.39f + 0.5f));
        ErrorMessage->SetLocalScale(Vector3f(5.0f));
        ErrorMessage->SetFontScale(0.5f);
        ErrorMessage->SetTextOffset(
                Vector3f(0.0f, -48 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f));
        ErrorMessage->SetImage(0, SURFACE_TEXTURE_DIFFUSE, ErrorIconTexture);
        ErrorMessage->SetVisible(false);

        // ==============================================================================
        //
        // sdcard icon
        //
        SDCardMessage = new UILabel(Cinema.GetGuiSys());
        SDCardMessage->AddToMenu(Menu, CenterRoot);
        SDCardMessage->SetLocalPose(forward, Vector3f(0.00f, 1.76f + 330.0f *
                                                                     VRMenuObject::DEFAULT_TEXEL_SCALE,
                                                      -7.39f + 0.5f));
        SDCardMessage->SetLocalScale(Vector3f(5.0f));
        SDCardMessage->SetFontScale(0.5f);
        SDCardMessage->SetTextOffset(
                Vector3f(0.0f, -96 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f));
        SDCardMessage->SetVisible(false);
        SDCardMessage->SetImage(0, SURFACE_TEXTURE_DIFFUSE, SDCardTexture);

        // ==============================================================================
        //
        // error without icon
        //
        PlainErrorMessage = new UILabel(Cinema.GetGuiSys());
        PlainErrorMessage->AddToMenu(Menu, CenterRoot);
        PlainErrorMessage->SetLocalPose(forward, Vector3f(0.00f, 1.76f + (330.0f - 48) *
                                                                         VRMenuObject::DEFAULT_TEXEL_SCALE,
                                                          -7.39f + 0.5f));
        PlainErrorMessage->SetLocalScale(Vector3f(5.0f));
        PlainErrorMessage->SetFontScale(0.5f);
        PlainErrorMessage->SetVisible(false);

        // ==============================================================================
        //
        // movie browser
        //
        MoviePanelPositions.push_back(PanelPose(forward, Vector3f(-5.59f, 1.76f, -12.55f),
                                               Vector4f(0.0f, 0.0f, 0.0f, 0.0f)));
        MoviePanelPositions.push_back(PanelPose(forward, Vector3f(-3.82f, 1.76f, -10.97f),
                                               Vector4f(0.1f, 0.1f, 0.1f, 1.0f)));
        MoviePanelPositions.push_back(PanelPose(forward, Vector3f(-2.05f, 1.76f, -9.39f),
                                               Vector4f(0.2f, 0.2f, 0.2f, 1.0f)));
        MoviePanelPositions.push_back(PanelPose(forward, Vector3f(0.00f, 1.76f, -7.39f),
                                               Vector4f(1.0f, 1.0f, 1.0f, 1.0f)));
        MoviePanelPositions.push_back(PanelPose(forward, Vector3f(2.05f, 1.76f, -9.39f),
                                               Vector4f(0.2f, 0.2f, 0.2f, 1.0f)));
        MoviePanelPositions.push_back(PanelPose(forward, Vector3f(3.82f, 1.76f, -10.97f),
                                               Vector4f(0.1f, 0.1f, 0.1f, 1.0f)));
        MoviePanelPositions.push_back(PanelPose(forward, Vector3f(5.59f, 1.76f, -12.55f),
                                               Vector4f(0.0f, 0.0f, 0.0f, 0.0f)));

        CenterIndex = MoviePanelPositions.size() / 2;
        CenterPosition = MoviePanelPositions[CenterIndex].Position;

        MovieBrowser = new CarouselBrowserComponent(MovieBrowserItems, MoviePanelPositions);
        MovieRoot->AddComponent(MovieBrowser);

        // ==============================================================================
        //
        // selection rectangle
        //
        SelectionFrame = new UIImage(Cinema.GetGuiSys());
        SelectionFrame->AddToMenu(Menu, MovieRoot);
        SelectionFrame->SetLocalPose(forward, CenterPosition + Vector3f(0.0f, 0.0f, 0.1f));
        SelectionFrame->SetLocalScale(PosterScale);
        SelectionFrame->SetImage(0, SURFACE_TEXTURE_DIFFUSE, SelectionTexture);
        SelectionFrame->AddComponent(new MovieSelectionComponent(this));

        const Vector3f selectionBoundsExpandMin = Vector3f(0.0f);
        const Vector3f selectionBoundsExpandMax = Vector3f(0.0f, -0.13f, 0.0f);
        SelectionFrame->GetMenuObject()->SetLocalBoundsExpand(selectionBoundsExpandMin,
                                                              selectionBoundsExpandMax);

        // ==============================================================================
        //
        // add shadow and 3D icon to movie poster panels
        //
        std::vector<VRMenuObject *> menuObjs;
        for (OVR::UPInt i = 0; i < MoviePanelPositions.size(); ++i) {
            UIContainer *posterContainer = new UIContainer(Cinema.GetGuiSys());
            posterContainer->AddToMenu(Menu, MovieRoot);
            posterContainer->SetLocalPose(MoviePanelPositions[i].Orientation,
                                          MoviePanelPositions[i].Position);
            posterContainer->GetMenuObject()->AddFlags(VRMENUOBJECT_FLAG_NO_FOCUS_GAINED);

            //
            // posters
            //
            UIImage *posterImage = new UIImage(Cinema.GetGuiSys());
            posterImage->AddToMenu(Menu, posterContainer);
            posterImage->SetLocalPose(forward, Vector3f(0.0f, 0.0f, 0.0f));
            posterImage->SetImage(0, SURFACE_TEXTURE_DIFFUSE, SelectionTexture.Texture,
                                  PosterWidth, PosterHeight);
            posterImage->SetLocalScale(PosterScale);
            posterImage->GetMenuObject()->AddFlags(VRMENUOBJECT_FLAG_NO_FOCUS_GAINED);
            posterImage->GetMenuObject()->SetLocalBoundsExpand(selectionBoundsExpandMin,
                                                               selectionBoundsExpandMax);

            if (i == CenterIndex) {
                CenterPoster = posterImage;
            }

            //
            // 3D icon
            //
            UIImage *is3DIcon = new UIImage(Cinema.GetGuiSys());
            is3DIcon->AddToMenu(Menu, posterContainer);
            is3DIcon->SetLocalPose(forward, Vector3f(0.75f, 1.3f, 0.02f));
            is3DIcon->SetImage(0, SURFACE_TEXTURE_DIFFUSE, Is3DIconTexture);
            is3DIcon->SetLocalScale(PosterScale);
            is3DIcon->GetMenuObject()->AddFlags(
                    VRMenuObjectFlags_t(VRMENUOBJECT_FLAG_NO_FOCUS_GAINED) |
                    VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));

            //
            // shadow
            //
            UIImage *shadow = new UIImage(Cinema.GetGuiSys());
            shadow->AddToMenu(Menu, posterContainer);
            shadow->SetLocalPose(forward, Vector3f(0.0f, -1.97f, 0.00f));
            shadow->SetImage(0, SURFACE_TEXTURE_DIFFUSE, ShadowTexture);
            shadow->SetLocalScale(PosterScale);
            shadow->GetMenuObject()->AddFlags(
                    VRMenuObjectFlags_t(VRMENUOBJECT_FLAG_NO_FOCUS_GAINED) |
                    VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));

            //
            // add the component
            //
            MoviePosterComponent *posterComp = new MoviePosterComponent();
            posterComp->SetMenuObjects(PosterWidth, PosterHeight, posterContainer, posterImage,
                                       is3DIcon, shadow);
            posterContainer->AddComponent(posterComp);

            menuObjs.push_back(posterContainer->GetMenuObject());
            MoviePosterComponents.push_back(posterComp);
        }

        MovieBrowser->SetMenuObjects(menuObjs, MoviePosterComponents);

        // ==============================================================================
        //
        // category browser
        //
        Categories.push_back(AppCategoryButton(CATEGORY_LIMELIGHT,
                                                Cinema.GetCinemaStrings().Category_LimeLight));


        // create the buttons and calculate their size
        const float itemWidth = 1.10f;
        float categoryBarWidth = 0.0f;
        for (OVR::UPInt i = 0; i < Categories.size(); ++i) {
            Categories[i].Button = new UILabel(Cinema.GetGuiSys());
            Categories[i].Button->AddToMenu(Menu, CategoryRoot);
            Categories[i].Button->SetFontScale(2.2f);
            Categories[i].Button->SetText(Categories[i].Text.c_str());
            Categories[i].Button->AddComponent(
                    new PcCategoryComponent(this, Categories[i].Category));

            const Bounds3f &bounds = Categories[i].Button->GetTextLocalBounds(
                    Cinema.GetGuiSys().GetDefaultFont());
            Categories[i].Width = std::max(bounds.GetSize().x, itemWidth) +
                                  80.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
            Categories[i].Height =
                    bounds.GetSize().y + 108.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
            categoryBarWidth += Categories[i].Width;
        }

        // reposition the buttons and set the background and border
        float startX = categoryBarWidth * -0.5f;
        for (OVR::UPInt i = 0; i < Categories.size(); ++i) {
            VRMenuSurfaceParms panelSurfParms("",
                                              BorderTexture.Texture, BorderTexture.Width,
                                              BorderTexture.Height, SURFACE_TEXTURE_ADDITIVE,
                                              0, 0, 0, SURFACE_TEXTURE_MAX,
                                              0, 0, 0, SURFACE_TEXTURE_MAX);

            panelSurfParms.Border = Vector4f(14.0f);
            panelSurfParms.Dims = Vector2f(Categories[i].Width * VRMenuObject::TEXELS_PER_METER,
                                           Categories[i].Height *
                                           VRMenuObject::TEXELS_PER_METER);

            Categories[i].Button->SetImage(0, panelSurfParms);
            Categories[i].Button->SetLocalPose(forward,
                                               Vector3f(startX + Categories[i].Width * 0.5f,
                                                        3.6f, -7.39f));
            Categories[i].Button->SetLocalBoundsExpand(Vector3f(0.0f, 0.13f, 0.0f),
                                                       Vector3f::ZERO);

            startX += Categories[i].Width;
        }

        // ==============================================================================
        //
        // movie title
        //
        MovieTitle = new UILabel(Cinema.GetGuiSys());
        MovieTitle->AddToMenu(Menu, TitleRoot);
        MovieTitle->SetLocalPose(forward, Vector3f(0.0f, 0.045f, -7.37f));
        MovieTitle->GetMenuObject()->AddFlags(VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));
        MovieTitle->SetFontScale(2.5f);

        // ==============================================================================
        //
        // swipe icons
        //
        float yPos = 1.76f - (PosterHeight - SwipeIconLeftTexture.Height) * 0.5f *
                             VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.y;

        for (int i = 0; i < NumSwipeTrails; i++) {
            Vector3f swipeIconPos = Vector3f(0.0f, yPos, -7.17f + 0.01f * (float) i);

            LeftSwipes[i] = new UIImage(Cinema.GetGuiSys());
            LeftSwipes[i]->AddToMenu(Menu, CenterRoot);
            LeftSwipes[i]->SetImage(0, SURFACE_TEXTURE_DIFFUSE, SwipeIconLeftTexture);
            LeftSwipes[i]->SetLocalScale(PosterScale);
            LeftSwipes[i]->GetMenuObject()->AddFlags(
                    VRMenuObjectFlags_t(VRMENUOBJECT_FLAG_NO_DEPTH) |
                    VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));
            LeftSwipes[i]->AddComponent(
                    new CarouselSwipeHintComponent(MovieBrowser, false, 1.3333f,
                                                   0.4f + (float) i * 0.13333f, 5.0f));

            swipeIconPos.x = ((PosterWidth + SwipeIconLeftTexture.Width * (i + 2)) * -0.5f) *
                             VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
            LeftSwipes[i]->SetLocalPosition(swipeIconPos);

            RightSwipes[i] = new UIImage(Cinema.GetGuiSys());
            RightSwipes[i]->AddToMenu(Menu, CenterRoot);
            RightSwipes[i]->SetImage(0, SURFACE_TEXTURE_DIFFUSE, SwipeIconRightTexture);
            RightSwipes[i]->SetLocalScale(PosterScale);
            RightSwipes[i]->GetMenuObject()->AddFlags(
                    VRMenuObjectFlags_t(VRMENUOBJECT_FLAG_NO_DEPTH) |
                    VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));
            RightSwipes[i]->AddComponent(
                    new CarouselSwipeHintComponent(MovieBrowser, true, 1.3333f,
                                                   0.4f + (float) i * 0.13333f, 5.0f));

            swipeIconPos.x = ((PosterWidth + SwipeIconRightTexture.Width * (i + 2)) * 0.5f) *
                             VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
            RightSwipes[i]->SetLocalPosition(swipeIconPos);
        }

        // ==============================================================================
        //
        // resume icon
        //
        ResumeIcon = new UILabel(Cinema.GetGuiSys());
        ResumeIcon->AddToMenu(Menu, MovieRoot);
        ResumeIcon->SetLocalPose(forward, CenterPosition + Vector3f(0.0f, 0.0f, 0.5f));
        ResumeIcon->SetImage(0, SURFACE_TEXTURE_DIFFUSE, ResumeIconTexture);
        ResumeIcon->GetMenuObject()->AddFlags(VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));
        ResumeIcon->SetFontScale(0.3f);
        ResumeIcon->SetLocalScale(6.0f);
        ResumeIcon->SetText(Cinema.GetCinemaStrings().MovieSelection_Resume.c_str());
        ResumeIcon->SetTextOffset(Vector3f(0.0f, -ResumeIconTexture.Height *
                                                 VRMenuObject::DEFAULT_TEXEL_SCALE * 0.5f,
                                           0.0f));
        ResumeIcon->SetVisible(false);

        // ==============================================================================
        //
        // close app button
        //
        CloseAppButton = new UIButton( Cinema.GetGuiSys() );
        CloseAppButton->AddToMenu(  Menu, MovieRoot );
        CloseAppButton->SetLocalPose( forward, CenterPosition + Vector3f( 0.8f, -1.28f, 0.5f ) );
        CloseAppButton->SetButtonImages( CloseIconTexture, CloseIconTexture, CloseIconTexture );
        CloseAppButton->SetLocalScale( 3.0f );
        CloseAppButton->SetVisible( false );
        CloseAppButton->SetOnClick( AppCloseAppButtonCallback, this );

        // ==============================================================================
        //
        // settings button
        //
        SettingsButton = new UIButton( Cinema.GetGuiSys() );
        SettingsButton->AddToMenu(  Menu, MovieRoot );
        SettingsButton->SetLocalPose( forward, CenterPosition + Vector3f( -0.8f, -1.28f, 0.5f ) );
        SettingsButton->SetButtonImages( SettingsIconTexture, SettingsIconTexture, SettingsIconTexture );
        SettingsButton->SetLocalScale( 3.0f );
        SettingsButton->SetVisible( true );
        SettingsButton->SetOnClick( SettingsButtonCallback, this );



        // ==============================================================================
        //
        // timer
        //
        TimerIcon = new UILabel(Cinema.GetGuiSys());
        TimerIcon->AddToMenu(Menu, MovieRoot);
        TimerIcon->SetLocalPose(forward, CenterPosition + Vector3f(0.0f, 0.0f, 0.5f));
        TimerIcon->GetMenuObject()->AddFlags(VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));
        TimerIcon->SetFontScale(1.0f);
        TimerIcon->SetLocalScale(2.0f);
        TimerIcon->SetText("10");
        TimerIcon->SetVisible(false);

        VRMenuSurfaceParms timerSurfaceParms("timer",
                                             "apk:///assets/timer.tga", SURFACE_TEXTURE_DIFFUSE,
                                             "apk:///assets/timer_fill.tga",
                                             SURFACE_TEXTURE_COLOR_RAMP_TARGET,
                                             "apk:///assets/color_ramp_timer.tga",
                                             SURFACE_TEXTURE_COLOR_RAMP);

        TimerIcon->SetImage(0, timerSurfaceParms);

        // text
        TimerText = new UILabel(Cinema.GetGuiSys());
        TimerText->AddToMenu(Menu, TimerIcon);
        TimerText->SetLocalPose(forward, Vector3f(0.0f, -0.3f, 0.0f));
        TimerText->GetMenuObject()->AddFlags(VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));
        TimerText->SetFontScale(1.0f);
        TimerText->SetText(Cinema.GetCinemaStrings().MovieSelection_Next);

        // ==============================================================================
        //
        // reorient message
        //
        MoveScreenLabel = new UILabel(Cinema.GetGuiSys());
        MoveScreenLabel->AddToMenu(Menu, NULL);
        MoveScreenLabel->SetLocalPose(forward, Vector3f(0.0f, 0.0f, -1.8f));
        MoveScreenLabel->GetMenuObject()->AddFlags(
                VRMenuObjectFlags_t(VRMENUOBJECT_DONT_HIT_ALL));
        MoveScreenLabel->SetFontScale(0.5f);
        MoveScreenLabel->SetText(Cinema.GetCinemaStrings().MoviePlayer_Reorient);
        MoveScreenLabel->SetTextOffset(Vector3f(0.0f, -24 * VRMenuObject::DEFAULT_TEXEL_SCALE,
                                                0.0f));  // offset to be below gaze cursor
        MoveScreenLabel->SetVisible(false);

        // ==============================================================================
        //
        // Settings
        //
        settingsMenu = new UIContainer( Cinema.GetGuiSys() );
        settingsMenu->AddToMenu(  Menu );
        settingsMenu->SetLocalPose( forward, Vector3f( 0.0f, 1.5f, -3.0f ) );
        settingsMenu->SetVisible(false);

        newPCbg.AddToMenu(  Menu, settingsMenu);
        newPCbg.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, 800, 1400 );

        static const float column1 = -0.40f;
        static const float column2 = 0.40f;
        static const float rowstart = 1.0f;
        static const float rowinc = -0.25f;
        float rowpos = rowstart;

        Button4k60 = new UIButton( Cinema.GetGuiSys() );
        Button4k60->AddToMenu(  Menu, settingsMenu );
        Button4k60->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
        Button4k60->SetText( Cinema.GetCinemaStrings().ButtonText_Button4k60.c_str() );
        TextButtonHelper(Button4k60);
        Button4k60->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button4k60->SetOnClick( SettingsCallback, this);
        Button4k60->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        Button4k30 = new UIButton( Cinema.GetGuiSys() );
        Button4k30->AddToMenu(  Menu, settingsMenu );
        Button4k30->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
        Button4k30->SetText( Cinema.GetCinemaStrings().ButtonText_Button4k30.c_str() );
        TextButtonHelper(Button4k30);
        Button4k30->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button4k30->SetOnClick( SettingsCallback, this);
        Button4k30->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        Button1080p60 = new UIButton( Cinema.GetGuiSys() );
        Button1080p60->AddToMenu(  Menu, settingsMenu );
        Button1080p60->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
        Button1080p60->SetText( Cinema.GetCinemaStrings().ButtonText_Button1080p60.c_str() );
        TextButtonHelper(Button1080p60);
        Button1080p60->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button1080p60->SetOnClick( SettingsCallback, this);
        Button1080p60->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        Button1080p30 = new UIButton( Cinema.GetGuiSys() );
        Button1080p30->AddToMenu(  Menu, settingsMenu );
        Button1080p30->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
        Button1080p30->SetText( Cinema.GetCinemaStrings().ButtonText_Button1080p30.c_str() );
        TextButtonHelper(Button1080p30);
        Button1080p30->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button1080p30->SetOnClick( SettingsCallback, this);
        Button1080p30->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        Button720p60 = new UIButton( Cinema.GetGuiSys() );
        Button720p60->AddToMenu( Menu, settingsMenu );
        Button720p60->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
        Button720p60->SetText( Cinema.GetCinemaStrings().ButtonText_Button720p60.c_str() );
        TextButtonHelper(Button720p60);
        Button720p60->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button720p60->SetOnClick( SettingsCallback, this);
        Button720p60->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        Button720p30 = new UIButton( Cinema.GetGuiSys() );
        Button720p30->AddToMenu( Menu, settingsMenu );
        Button720p30->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
        Button720p30->SetText( Cinema.GetCinemaStrings().ButtonText_Button720p30.c_str() );
        TextButtonHelper(Button720p30);
        Button720p30->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button720p30->SetOnClick( SettingsCallback, this);
        Button720p30->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        // skip 1/4 a space
        rowpos += rowinc /4;

        /*ButtonHostAudio = new UIButton( Cinema.GetGuiSys() );
        ButtonHostAudio->AddToMenu( Menu, settingsMenu );
        ButtonHostAudio->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
        ButtonHostAudio->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonHostAudio.c_str() );
        TextButtonHelper(ButtonHostAudio);
        ButtonHostAudio->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        ButtonHostAudio->SetOnClick( SettingsCallback, this);*/
        //ButtonHostAudio->SetSelected(SettingsSelectedCallback(ButtonHostAudio,this));
        //ButtonHostAudio->SetIsSelected( SettingsSelectedCallback, this);

        // restart next column
        rowpos = rowstart;

        ButtonGaze = new UIButton( Cinema.GetGuiSys() );
        ButtonGaze->AddToMenu( Menu, settingsMenu );
        ButtonGaze->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
        ButtonGaze->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonGaze.c_str() );
        TextButtonHelper(ButtonGaze);
        ButtonGaze->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        ButtonGaze->SetOnClick( SettingsCallback, this);
        ButtonGaze->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        ButtonTrackpad = new UIButton( Cinema.GetGuiSys() );
        ButtonTrackpad->AddToMenu( Menu, settingsMenu );
        ButtonTrackpad->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
        ButtonTrackpad->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonTrackpad.c_str() );
        TextButtonHelper(ButtonTrackpad);
        ButtonTrackpad->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        ButtonTrackpad->SetOnClick( SettingsCallback, this);
        ButtonTrackpad->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        ButtonOff = new UIButton( Cinema.GetGuiSys() );
        ButtonOff->AddToMenu( Menu, settingsMenu );
        ButtonOff->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
        ButtonOff->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonOff.c_str() );
        TextButtonHelper(ButtonOff);
        ButtonOff->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        ButtonOff->SetOnClick( SettingsCallback, this);
        ButtonOff->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        // skip half a space
        rowpos += rowinc /2;

        Button169 = new UIButton( Cinema.GetGuiSys() );
        Button169->AddToMenu( Menu, settingsMenu );
        Button169->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
        Button169->SetText( "16:9" );
        TextButtonHelper(Button169);
        Button169->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button169->SetOnClick( SettingsCallback, this);
        Button169->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        Button43 = new UIButton( Cinema.GetGuiSys() );
        Button43->AddToMenu( Menu, settingsMenu );
        Button43->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
        Button43->SetText( "4:3" );
        TextButtonHelper(Button43);
        Button43->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        Button43->SetOnClick( SettingsCallback, this);
        Button43->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

        // skip half a space
        rowpos += rowinc /2;

        ButtonSaveApp = new UIButton( Cinema.GetGuiSys() );
        ButtonSaveApp->AddToMenu( Menu, settingsMenu );
        ButtonSaveApp->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
        ButtonSaveApp->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSaveApp.c_str() );
        TextButtonHelper(ButtonSaveApp);
        ButtonSaveApp->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        ButtonSaveApp->SetOnClick( SettingsCallback, this );

        ButtonSaveDefault = new UIButton( Cinema.GetGuiSys() );
        ButtonSaveDefault->AddToMenu( Menu, settingsMenu );
        ButtonSaveDefault->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
        ButtonSaveDefault->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSaveDefault.c_str() );
        TextButtonHelper(ButtonSaveDefault);
        ButtonSaveDefault->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
        ButtonSaveDefault->SetOnClick( SettingsCallback, this);

    }

    void AppSelectionView::CloseAppButtonPressed()
    {
        const PcDef* pc = Cinema.GetCurrentPc();
        const AppDef* app = NULL;

        int selected = MovieBrowser->GetSelection();
        if(selected <   static_cast< int >(AppList.size()))
        {
            app = AppList[selected];
        }
        if(pc && app)
        {
            Native::closeApp(  pc->UUID.c_str(), app->Id);
        }
    }

    void AppSelectionView::SettingsButtonPressed()
    {
        int appIndex = MovieBrowser->GetSelection();

        const AppDef* app = AppList[appIndex];

        std::string    outPath;
        const ovrJava & java = *reinterpret_cast< const ovrJava* >( Cinema.GetContext()->ContextForVrApi() );
        const bool validDir =ovrFileSys::GetPathIfValidPermission( java,EST_INTERNAL_STORAGE, EFT_FILES, "", permissionFlags_t( PERMISSION_READ ) | permissionFlags_t( PERMISSION_WRITE ), outPath );

        if(validDir)
        {
            if(defaultSettings == NULL)
            {
                defaultSettingsPath = outPath + "settings.json";
                defaultSettings = new Settings(defaultSettingsPath.c_str());

                defaultSettings->Define("StreamTheaterSettingsVersion", &settingsVersion);
                defaultSettings->Define("MouseMode", (int*)&mouseMode);
                defaultSettings->Define("StreamAspectRatio", (int*)&streamAspectRatio);
                defaultSettings->Define("StreamWidth", &streamWidth);
                defaultSettings->Define("StreamHeight", &streamHeight);
                defaultSettings->Define("StreamFPS", &streamFPS);
                defaultSettings->Define("EnableHostAudio", &streamHostAudio);
            }

            if(appSettings != NULL)
            {
                delete(appSettings);
                appSettings = NULL;
            }

            appSettingsPath = outPath + "settings." + app->Name + ".json";
            appSettings = new Settings(appSettingsPath.c_str());
            appSettings->CopyDefines(*defaultSettings);

            defaultSettings->Load();
            appSettings->Load();
        }

        ButtonGaze->UpdateButtonState();
        ButtonTrackpad->UpdateButtonState();
        ButtonOff->UpdateButtonState();
        Button169->UpdateButtonState();
        Button43->UpdateButtonState();
        Button4k60->UpdateButtonState();
        Button4k30->UpdateButtonState();
        Button1080p30->UpdateButtonState();
        Button1080p60->UpdateButtonState();
        Button1080p30->UpdateButtonState();
        Button720p60->UpdateButtonState();
        Button720p30->UpdateButtonState();
        //ButtonHostAudio->UpdateButtonState();
        ButtonSaveApp->UpdateButtonState();
        ButtonSaveDefault->UpdateButtonState();

        settingsMenu->SetVisible(true);
    }

    void AppSelectionView::SettingsPressed( UIButton *button)
    {
        if( button == ButtonGaze )
        {
            mouseMode = MOUSE_GAZE;
        }
        else if( button ==ButtonTrackpad )
        {
            mouseMode = MOUSE_TRACKPAD;
        }
        else if( button == ButtonOff )
        {
            mouseMode = MOUSE_OFF;
        }
        else if( button == Button4k60 )
        {
            streamWidth = 3840; streamHeight = 2160; streamFPS = 60;
        }
        else if( button == Button4k30 )
        {
            streamWidth = 3840; streamHeight = 2160; streamFPS = 30;
        }
        else if( button == Button169 )
        {
            streamAspectRatio = DIECISEIS_NOVENOS;
        }
        else if( button == Button43 )
        {
            streamAspectRatio = CUATRO_TERCIOS;
        }
        else if( button == Button1080p60 )
        {
            streamWidth = 1920; streamHeight = 1080; streamFPS = 60;
        }
        else if( button == Button1080p30 )
        {
            streamWidth = 1920; streamHeight = 1080; streamFPS = 30;
        }
        else if( button == Button720p60 )
        {
            streamWidth = 1280; streamHeight = 720; streamFPS = 60;
        }
        else if( button == Button720p30 )
        {
            streamWidth = 1280; streamHeight = 720; streamFPS = 30;
        }
        else if( button == ButtonHostAudio )
        {
            streamHostAudio = !streamHostAudio;
        }
        else if( button == ButtonSaveApp )
        {
            appSettings->SaveChanged();
        }
        else if( button == ButtonSaveDefault )
        {
            defaultSettings->SaveAll();
        }

        ButtonGaze->UpdateButtonState();
        ButtonTrackpad->UpdateButtonState();
        ButtonOff->UpdateButtonState();
        Button169->UpdateButtonState();
        Button43->UpdateButtonState();
        Button4k60->UpdateButtonState();
        Button4k30->UpdateButtonState();
        Button1080p60->UpdateButtonState();
        Button1080p30->UpdateButtonState();
        Button720p60->UpdateButtonState();
        Button720p30->UpdateButtonState();
        //ButtonHostAudio->UpdateButtonState();
        ButtonSaveApp->UpdateButtonState();
        ButtonSaveDefault->UpdateButtonState();
    }

bool AppSelectionView::SettingsIsActive( UIButton *button)
{
    if( button == ButtonSaveApp )
    {
        return appSettings->IsChanged();
    }
    else if( button == ButtonSaveDefault )
    {
        return defaultSettings->IsChanged();
    }
    return false;
}

    Vector3f AppSelectionView::ScalePosition(const Vector3f &startPos, const float scale,
                                             const float menuOffset) const {
        const float eyeHieght = Cinema.SceneMgr.Scene.GetEyeHeight();

        Vector3f pos = startPos;
        pos.x *= scale;
        pos.y = (pos.y - eyeHieght) * scale + eyeHieght + menuOffset;
        pos.z *= scale;
        pos += Cinema.SceneMgr.Scene.GetFootPos();

        return pos;
    }

    bool AppSelectionView::BackPressed()
    {
        if(ErrorShown())
        {
            ClearError();
            return true;
        }

        if(settingsMenu->GetVisible())
        {
            settingsMenu->SetVisible(false);

            return true;
        }

        //Cinema.PcSelection(true);
        return false;
    }



    bool AppSelectionView::OnKeyEvent(const int keyCode, const int repeatCount,
                                      const UIKeyboard::KeyEventType eventType) {
        OVR_UNUSED(keyCode);
        OVR_UNUSED(repeatCount);
        OVR_UNUSED(eventType);
        switch ( keyCode ) {/*
            case OVR_KEY_BACK: {
                switch (eventType) {
                    case KEY_EVENT_SHORT_PRESS:
                        OVR_LOG("KEY_EVENT_SHORT_PRESS");
                        BackPressed();
                        return true;
                        break;
                    default:
                        //OVR_LOG( "unexpected back key state %i", eventType );
                        break;
                }
            }*/
        }
        return false;
    }

    void AppSelectionView::UpdateMenuPosition() {
        // scale down when in a theater
        const float scale = Cinema.InLobby ? 1.0f : 0.55f;
        CenterRoot->GetMenuObject()->SetLocalScale(Vector3f(scale));

        if (!Cinema.InLobby && Cinema.SceneMgr.SceneInfo.UseFreeScreen) {
            Quatf orientation = Quatf(Cinema.SceneMgr.FreeScreenPose);
            CenterRoot->GetMenuObject()->SetLocalRotation(orientation);
            CenterRoot->GetMenuObject()->SetLocalPosition(
                    Cinema.SceneMgr.FreeScreenPose.Transform(
                            Vector3f(0.0f, -1.76f * scale, 0.0f)));
        } else {
            const float menuOffset = Cinema.InLobby ? 0.0f : 0.5f;
            CenterRoot->GetMenuObject()->SetLocalRotation(Quatf());
            CenterRoot->GetMenuObject()->SetLocalPosition(
                    ScalePosition(Vector3f::ZERO, scale, menuOffset));
        }
    }

    void AppSelectionView::Select() {
        OVR_LOG( "AppSelectionView::Select");

        // ignore selection while repositioning screen
        if (RepositionScreen) {
            return;
        }

        int lastIndex = AppIndex;
        AppIndex = MovieBrowser->GetSelection();
        Cinema.SetPlaylist(AppList, AppIndex);

        Cinema.InLobby = true;
        if (!Cinema.InLobby) {
            if (lastIndex == AppIndex) {
                // selected the poster we were just watching
                Cinema.PlayOrResumeOrRestartApp();
            } else {
                Cinema.PlayOrResumeOrRestartApp();
            }
        } else {
            Cinema.TheaterSelection();
        }
    }

    void AppSelectionView::StartTimer() {
        const double now = vrapi_GetTimeInSeconds();
        TimerStartTime = now;
        TimerValue = -1;
        ShowTimer = true;
    }





    void AppSelectionView::SetCategory(const PcCategory category) {
        // default to category in index 0
        OVR::UPInt categoryIndex = 0;
        for (OVR::UPInt i = 0; i < Categories.size(); ++i) {
            if (category == Categories[i].Category) {
                categoryIndex = i;
                break;
            }
        }

        OVR_LOG("SetCategory: %s", Categories[categoryIndex].Text.c_str());
        CurrentCategory = Categories[categoryIndex].Category;
        for (OVR::UPInt i = 0; i < Categories.size(); ++i) {
            Categories[i].Button->SetHilighted(i == categoryIndex);
        }

        // reset all the swipe icons so they match the current poster
        for (int i = 0; i < NumSwipeTrails; i++) {
            CarouselSwipeHintComponent *compLeft = LeftSwipes[i]->GetMenuObject()->GetComponentByTypeName<CarouselSwipeHintComponent>();
            compLeft->Reset(LeftSwipes[i]->GetMenuObject());
            CarouselSwipeHintComponent *compRight = RightSwipes[i]->GetMenuObject()->GetComponentByTypeName<CarouselSwipeHintComponent>();
            compRight->Reset(RightSwipes[i]->GetMenuObject());
        }
        Cinema.AppMgr.LoadPosters();
        SetAppList(Cinema.AppMgr.GetAppList(CurrentCategory),  NULL);

        OVR_LOG("%zu movies added", AppList.size());
    }

    void AppSelectionView::SetAppList(const std::vector<const AppDef *> &apps,
                                      const AppDef *nextApp) {
        //OVR_LOG( "AppSelectionView::SetAppList: %d movies", movies.size());

        AppList = apps;
        DeletePointerArray(MovieBrowserItems);
        for (OVR::UPInt i = 0; i < AppList.size(); i++) {
            const AppDef *app = AppList[i];

            OVR_LOG( "AddMovie: %s", app->Name.c_str());

            CarouselItem *item = new CarouselItem();
            item->Texture = app->Poster;
            item->TextureWidth = app->PosterWidth;
            item->TextureHeight = app->PosterHeight;
            MovieBrowserItems.push_back(item);
        }
        MovieBrowser->SetItems(MovieBrowserItems);

        MovieTitle->SetText("");
        LastAppDisplayed = NULL;

        //AppIndex = 0;
        if (nextApp != NULL) {
            for (OVR::UPInt i = 0; i < AppList.size(); i++) {
                if (apps[i] == nextApp) {
                    StartTimer();
                    AppIndex = i;
                    break;
                }
            }
        }

        MovieBrowser->SetSelectionIndex(AppIndex);

        if (AppList.size() == 0) {
            if (CurrentCategory == CATEGORY_LIMELIGHT) {
                SetError("error", false,
                         false);
                ErrorMessageClicked = true;
            } else {
                SetError("error", true,
                         false);
            }
        } else {
            ClearError();
        }
    }

    void AppSelectionView::SelectionHighlighted(bool isHighlighted) {
        if (isHighlighted && !ShowTimer && !Cinema.InLobby &&
            (AppIndex == MovieBrowser->GetSelection())) {
            // dim the poster when the resume icon is up and the poster is highlighted
            CenterPoster->SetColor(Vector4f(0.55f, 0.55f, 0.55f, 1.0f));
        } else if (MovieBrowser->HasSelection()) {
            CenterPoster->SetColor(Vector4f(1.0f));
        }
    }

    void AppSelectionView::UpdateSelectionFrame(const ovrApplFrameIn &vrFrame) {
        const double now = vrapi_GetTimeInSeconds();
        if (!MovieBrowser->HasSelection()) {
            SelectionFader.Set(now, 0, now + 0.1, 1.0f);
            TimerStartTime = 0;
        }

        if (!SelectionFrame->GetMenuObject()->IsHilighted()) {
            SelectionFader.Set(now, 0, now + 0.1, 1.0f);
        } else {
            MovieBrowser->CheckGamepad(Cinema.GetGuiSys(), vrFrame, MovieRoot->GetMenuObject());
        }

        SelectionFrame->SetColor(Vector4f(static_cast<float>( SelectionFader.Value(now))));

        int selected = MovieBrowser->GetSelection();
        if ( selected >= 0 && static_cast<int>(AppList.size()) > selected && AppList[ selected ]->isRunning ){
            ResumeIcon->SetColor(Vector4f(static_cast<float>( SelectionFader.Value(now))));
            ResumeIcon->SetTextColor(Vector4f(static_cast<float>( SelectionFader.Value(now))));
            ResumeIcon->SetVisible(true);
            CloseAppButton->SetVisible( true );
        } else {
            ResumeIcon->SetVisible(false);
            CloseAppButton->SetVisible( false );
        }

        if (ShowTimer && (TimerStartTime != 0)) {
            double frac = (now - TimerStartTime) / TimerTotalTime;
            if (frac > 1.0f) {
                frac = 1.0f;
                Cinema.SetPlaylist(AppList, MovieBrowser->GetSelection());
                Cinema.PlayOrResumeOrRestartApp();
            }
            Vector2f offset(0.0f, 1.0f - static_cast<float>( frac ));
            TimerIcon->SetColorTableOffset(offset);

            int seconds = static_cast<int>( TimerTotalTime - (TimerTotalTime * frac));
            if (TimerValue != seconds) {
                TimerValue = seconds;
                const char *text = std::to_string(seconds).c_str();
                TimerIcon->SetText(text);
            }
            TimerIcon->SetVisible(true);
            CenterPoster->SetColor(Vector4f(0.55f, 0.55f, 0.55f, 1.0f));
        } else {
            TimerIcon->SetVisible(false);
        }
    }

    void AppSelectionView::SetError(const char *text, bool showSDCard, bool showErrorIcon) {
        ClearError();

        OVR_LOG("SetError: %s", text);
        if (showSDCard) {
            SDCardMessage->SetVisible(true);
            SDCardMessage->SetTextWordWrapped(text, Cinema.GetGuiSys().GetDefaultFont(), 1.0f);
        } else if (showErrorIcon) {
            ErrorMessage->SetVisible(true);
            ErrorMessage->SetTextWordWrapped(text, Cinema.GetGuiSys().GetDefaultFont(), 1.0f);
        } else {
            PlainErrorMessage->SetVisible(true);
            PlainErrorMessage->SetTextWordWrapped(text, Cinema.GetGuiSys().GetDefaultFont(),
                                                  1.0f);
        }
        TitleRoot->SetVisible(false);
        MovieRoot->SetVisible(false);

        CarouselSwipeHintComponent::ShowSwipeHints = false;
    }

    void AppSelectionView::ClearError() {
        OVR_LOG("ClearError");
        ErrorMessageClicked = false;
        ErrorMessage->SetVisible(false);
        SDCardMessage->SetVisible(false);
        PlainErrorMessage->SetVisible(false);
        TitleRoot->SetVisible(true);
        MovieRoot->SetVisible(true);

        CarouselSwipeHintComponent::ShowSwipeHints = true;
    }

    bool AppSelectionView::ErrorShown() const {
        return ErrorMessage->GetVisible() || SDCardMessage->GetVisible() ||
               PlainErrorMessage->GetVisible();
    }
}
