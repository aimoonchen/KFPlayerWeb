
#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"

//#define SERVER
#define URHO3D_LUA

#include <EASTL/sort.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/ApplicationSettings.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/SceneManager.h>

#ifdef URHO3D_LUA
#include <Urho3D/LuaScript/LuaScript.h>
#include <Urho3D/sol/sol.hpp>
#endif
// #if URHO3D_CSHARP
// #include <Urho3D/Script/Script.h>
// #endif
// #if URHO3D_SYSTEMUI
// #include <Urho3D/SystemUI/SystemUI.h>
// #endif
// #if URHO3D_RMLUI
// #include <Urho3D/RmlUI/RmlUI.h>
// #endif

namespace Urho3D
{
class CacheRouter : public ResourceRouter
{
    URHO3D_OBJECT(CacheRouter, ResourceRouter);
public:
    /// Construct.
    explicit CacheRouter(Context* context);
    ///
    void Route(ea::string& name, ResourceRequest requestType) override;
    ///
    bool AddPackage(PackageFile* packageFile);

private:
    ///
    ea::unordered_map<ea::string, ea::string> mapping_;
};

CacheRouter::CacheRouter(Context* context)
    : ResourceRouter(context)
{
}

void CacheRouter::Route(ea::string& name, ResourceRequest requestType)
{
    auto it = mapping_.find(name);
    if (it != mapping_.end())
        name = it->second;
}

bool CacheRouter::AddPackage(PackageFile* packageFile)
{
    if (packageFile == nullptr)
        return false;

    File file(context_);
    const char* cacheInfo = "CacheInfo.json";
    if (!file.Open(packageFile, cacheInfo))
    {
        URHO3D_LOGERROR("Failed to open {} in package {}", cacheInfo, packageFile->GetName());
        return false;
    }

    JSONFile jsonFile(context_);
    if (!jsonFile.BeginLoad(file))
    {
        URHO3D_LOGERROR("Failed to load {} in package {}", cacheInfo, packageFile->GetName());
        return false;
    }

    // TODO: Revisit
    JSONInputArchive archive(&jsonFile);
    ea::unordered_map<ea::string, ea::string> mapping;
    SerializeMap(archive, "cacheInfo", mapping, "map");

    mapping_.insert(mapping.begin(), mapping.end());
    return true;
}

class Player : public Application
{
public:
    ///
    explicit Player(Context* context);
    ///
    void Setup() override;
    ///
    void Start() override;
    ///
    void Stop() override;

protected:
    ///
    virtual bool LoadPlugins(const StringVector& plugins);
#if URHO3D_PLUGINS
    ///
    bool LoadAssembly(const ea::string& path);
    ///
    bool RegisterPlugin(PluginApplication* plugin);
#endif

    struct LoadedModule
    {
        SharedPtr<PluginModule> module_;
        SharedPtr<PluginApplication> application_;
    };

    ///
    ApplicationSettings settings_{context_};
    ///
    ea::vector<LoadedModule> plugins_;
    ///
    CacheRouter cacheRouter_{context_};

private:
    /// Handle reload start of the script file.
    void HandleScriptReloadStarted(StringHash eventType, VariantMap& eventData);
    /// Handle reload success of the script file.
    void HandleScriptReloadFinished(StringHash eventType, VariantMap& eventData);
    /// Handle reload failure of the script file.
    void HandleScriptReloadFailed(StringHash eventType, VariantMap& eventData);
    /// Parse script file name from the first argument.
    void GetScriptFileName();

    /// Script file name.
    ea::string scriptFileName_;
};

Player::Player(Context* context)
    : Application(context)
{
}

void Player::Setup()
{
    FileSystem* fs = context_->GetSubsystem<FileSystem>();
    auto& engineParameters_ = GetEngineParameters();
#ifndef _WIN32
    engineParameters_[EP_RESOURCE_PATHS] = "Assets/Engine";// "CoreData;Data";//
#endif
// #if MOBILE
//     engineParameters_[EP_RESOURCE_PATHS] = "";
// #else
//     engineParameters_[EP_RESOURCE_PREFIX_PATHS] = fs->GetProgramDir() + ";" + fs->GetCurrentDir();
// #endif

#if DESKTOP && URHO3D_DEBUG
    // Developer builds. Try loading from filesystem first.
    const ea::string settingsFilePath = "Cache/Settings.json";
    if (context_->GetSubsystem<FileSystem>()->Exists(settingsFilePath))
    {
        JSONFile jsonFile(context_);
        if (jsonFile.LoadFile(settingsFilePath))
        {
            JSONInputArchive archive(&jsonFile);
            SerializeValue(archive, "settings", settings_);

            for (const auto& pair : settings_.engineParameters_)
                engineParameters_[pair.first] = pair.second;
            engineParameters_[EP_RESOURCE_PATHS] = "Cache;Resources";
            // Unpacked application is executing. Do not attempt to load paks.
            URHO3D_LOGINFO("This is a developer build executing unpacked application.");
            return;
        }
    }
#endif

    const ea::string defaultPak("Resources-default.pak");

    // Enumerate pak files
    StringVector pakFiles;
#if ANDROID
    const ea::string scanDir = ea::string(APK);
#else
    const ea::string scanDir = fs->GetCurrentDir();
#endif
    fs->ScanDir(pakFiles, scanDir, "*.pak", SCAN_FILES, false);

    // Sort paks. If something funny happens at least we will have a deterministic behavior.
    ea::quick_sort(pakFiles.begin(), pakFiles.end());

    // Default pak file goes to the front of the list. It is available on all platforms, but we may desire that
    // platform-specific pak would override settings defined in main pak.
    auto it = pakFiles.find(defaultPak);
    if (it != pakFiles.end())
    {
        pakFiles.erase(it);
        pakFiles.push_front(defaultPak);
    }

    // Find pak file for current platform
    for (const ea::string& pakFile : pakFiles)
    {
        SharedPtr<PackageFile> package(new PackageFile(context_));
        if (!package->Open(APK + pakFile))
        {
            URHO3D_LOGERROR("Failed to open {}", pakFile);
            continue;
        }

        if (!package->Exists("Settings.json"))
            // Likely a custom pak file from user. User is responsible for loading it.
            continue;

        SharedPtr<File> file(new File(context_));
        if (!file->Open(package, "Settings.json"))
        {
            URHO3D_LOGERROR("Unable to open Settings.json in {}", pakFile);
            continue;
        }

        JSONFile jsonFile(context_);
        if (!jsonFile.BeginLoad(*file))
        {
            URHO3D_LOGERROR("Unable to load Settings.json in {}", pakFile);
            continue;
        }

        JSONInputArchive archive(&jsonFile);
        SerializeValue(archive, "settings", settings_);

        // Empty platforms list means pak is meant for all platforms.
        if (settings_.platforms_.empty() || settings_.platforms_.contains(GetPlatform()))
        {
            // Paks are manually added here in order to avoid modifying EP_RESOURCE_PACKAGES value. User may specify
            // this configuration parameter to load any custom paks if desired. Engine will add them later. Besides we
            // already had to to open and parse package in order to find Settings.json. By adding paks now we avoid
            // engine doing all the loading same file twice.
            context_->GetSubsystem<ResourceCache>()->AddPackageFile(package);
            for (const auto& pair : settings_.engineParameters_)
                engineParameters_[pair.first] = pair.second;
        }
    }
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..;../..";
    engineParameters_[EP_FULL_SCREEN] = false;
    engineParameters_[EP_WINDOW_WIDTH] = 1280;
    engineParameters_[EP_WINDOW_HEIGHT] = 720;
#ifdef SERVER
    engineParameters_[EP_SOUND] = false;
    engineParameters_[EP_HEADLESS] = true;
#endif
}

void Player::Start()
{
// #if URHO3D_SYSTEMUI
//     ui::GetIO().IniFilename = nullptr; // Disable of imgui.ini creation,
// #endif

    // Add resource router that maps cooked resources to their original resource names.
    auto* router = new CacheRouter(context_);
    for (PackageFile* package : context_->GetSubsystem<ResourceCache>()->GetPackageFiles())
    {
        if (package->Exists("CacheInfo.json"))
            router->AddPackage(package);
    }
    context_->GetSubsystem<ResourceCache>()->AddResourceRouter(router);

    context_->RegisterSubsystem(new SceneManager(context_));

// #if URHO3D_RMLUI
//     auto* cache = GetSubsystem<ResourceCache>();
//     auto* ui = GetSubsystem<RmlUI>();
//     ea::vector<ea::string> fonts;
//     cache->Scan(fonts, "Fonts/", "*.ttf", SCAN_FILES, true);
//     cache->Scan(fonts, "Fonts/", "*.otf", SCAN_FILES, true);
//     for (const ea::string& font : fonts)
//         ui->LoadFont(Format("Fonts/{}", font));
// #endif

#if URHO3D_STATIC
    // Static builds require user to manually register plugins by subclassing Player class.
    SendEvent(E_REGISTERSTATICPLUGINS);
#else
    // Shared builds load plugin .so/.dll specified in config file.
    if (!LoadPlugins(settings_.plugins_))
        ErrorExit("Loading of required plugins failed.");
#endif
#if URHO3D_CSHARP && URHO3D_PLUGINS
    // Internal plugin which handles complication of *.cs source code in resource path.
    RegisterPlugin(Script::GetRuntimeApi()->CompileResourceScriptPlugin());
#endif

    for (LoadedModule& plugin : plugins_)
    {
        plugin.application_->SendEvent(E_PLUGINSTART);
        plugin.application_->Start();
    }

    // Load main scene.
    //     {
    //         auto* manager = GetSubsystem<SceneManager>();
    //         Scene* scene = manager->CreateScene();
    //
    //         if (scene->LoadFile(settings_.defaultScene_))
    //             manager->SetActiveScene(scene);
    //         else
    //             ErrorExit("Invalid scene file.");
    //     }

    // Check for script file name from the arguments
    GetScriptFileName();

    if (scriptFileName_.empty())
    {
        ErrorExit("Script file name not specified; cannot proceed");
        return;
    }
    ea::string extension = GetExtension(scriptFileName_);
    if (extension != ".lua" && extension != ".luc")
    {
    }
    else
    {
#ifdef URHO3D_LUA
        // Instantiate and register the Lua script subsystem
        auto* luaScript = new LuaScript(context_);
        context_->RegisterSubsystem(luaScript);

        // If script loading is successful, proceed to main loop
        if (luaScript->ExecuteFile(scriptFileName_))
        {
            // luaScript->ExecuteFunction("Start");
            sol::protected_function_result result = luaScript->ExecuteFunction("Start");
            if (!result.valid())
            {
                sol::error err = result;
                sol::call_status status = result.status();
                URHO3D_LOGERRORF("%s error\n\t%s", sol::to_string(status).c_str(), err.what());
            }
            return;
        }
        //         auto lua = luaScript->GetState();
        //         sol::load_result loaded_chunk = lua->load(scriptFileName_.CString());
        //         if (!loaded_chunk.valid())
        //         {
        //             sol::error err = loaded_chunk;
        //             sol::load_status status = loaded_chunk.status();
        // //             std::cout << "Something went horribly wrong loading the code: " << sol::to_string(status) << "
        // error"
        // //                       << "\n\t" << err.what() << std::endl;
        //             std::string status_str = sol::to_string(status);
        //             URHO3D_LOGERRORF("load script file : %s  Error\n\t%s", status_str.c_str(), err.what());
        //         }
        //         else
        //         {
        //             // Because the syntax is bad, this will never be reached
        //             //c_assert(false);
        //             // If there is a runtime error (lua GC memory error, nil access, etc.)
        //             // it will be caught here
        //             sol::protected_function script_func = loaded_chunk;
        //             sol::protected_function_result result = script_func();
        //             if (!result.valid())
        //             {
        //                 sol::error err = result;
        //                 sol::call_status status = result.status();
        // //                 std::cout << "Something went horribly wrong running the code: " << sol::to_string(status)
        // << " error"
        // //                           << "\n\t" << err.what() << std::endl;
        //                 std::string status_str = sol::to_string(status);
        //
        //                 URHO3D_LOGERRORF("run lua code : %s  Error\n\t%s", status_str.c_str(), err.what());
        //             }
        //         }
#else
        ErrorExit("Lua is not enabled!");
        return;
#endif
    }

    // The script was not successfully loaded. Show the last error message and do not run the main loop
    ErrorExit();
}

void Player::Stop()
{
    for (LoadedModule& plugin : plugins_)
    {
        plugin.application_->SendEvent(E_PLUGINSTOP);
        plugin.application_->Stop();
    }

    if (auto* manager = GetSubsystem<SceneManager>())
        manager->UnloadAll();

    for (LoadedModule& plugin : plugins_)
    {
        plugin.application_->SendEvent(E_PLUGINUNLOAD);
        plugin.application_->Unload();
    }

#if URHO3D_CSHARP
    for (LoadedModule& plugin : plugins_)
        Script::GetRuntimeApi()->DereferenceAndDispose(plugin.application_.Detach());
#endif
}

bool Player::LoadPlugins(const StringVector& plugins)
{
    // Load plugins.
#if URHO3D_PLUGINS
    for (const ea::string& pluginName : plugins)
    {
        ea::string pluginFileName;
        bool loaded = false;
#if !_WIN32
        // Native plugins on unixes
#if LINUX
        pluginFileName = "lib" + pluginName + ".so";
#elif APPLE
        pluginFileName = "lib" + pluginName + ".dylib";
#endif

#if MOBILE
        // On mobile libraries are loaded already so it is ok to not check for existence, TODO: iOS
        loaded = LoadAssembly(pluginFileName);
#else
        // On desktop we can access file system as usual
        if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
            loaded = LoadAssembly(pluginFileName);
        else
        {
            pluginFileName = context_->GetSubsystem<FileSystem>()->GetProgramDir() + pluginFileName;
            if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
                loaded = LoadAssembly(pluginFileName);
        }
#endif // MOBILE
#endif // !_WIN32

#if _WIN32 || URHO3D_CSHARP
        // Native plugins on windows or managed plugins on all platforms
        if (!loaded)
        {
            pluginFileName = pluginName + ".dll";
#if ANDROID
            pluginFileName = ea::string(APK) + "assets/.net/" + pluginFileName;
#endif
            if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
                loaded = LoadAssembly(pluginFileName);
#if DESKTOP
            else
            {
                pluginFileName = context_->GetSubsystem<FileSystem>()->GetProgramDir() + pluginFileName;
                if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
                    loaded = LoadAssembly(pluginFileName);
            }
#endif // DESKTOP
        }
#endif // URHO3D_PLUGINS

        if (!loaded)
        {
            URHO3D_LOGERRORF("Loading of '%s' assembly failed.", pluginName.c_str());
            return false;
        }
    }
#endif // URHO3D_PLUGINS
    return true;
}
#if URHO3D_PLUGINS
bool Player::LoadAssembly(const ea::string& path)
{
    LoadedModule moduleInfo;

    moduleInfo.module_ = new PluginModule(context_);
    if (moduleInfo.module_->Load(path))
    {
        moduleInfo.application_ = moduleInfo.module_->InstantiatePlugin();
        if (moduleInfo.application_.NotNull())
        {
            plugins_.emplace_back(moduleInfo);
            moduleInfo.application_->SendEvent(E_PLUGINLOAD);
            moduleInfo.application_->Load();
            return true;
        }
    }
    return false;
}

bool Player::RegisterPlugin(PluginApplication* plugin)
{
    if (plugin == nullptr)
    {
        URHO3D_LOGERROR("PluginApplication may not be null.");
        return false;
    }
    LoadedModule moduleInfo;
    moduleInfo.application_ = plugin;
    plugins_.emplace_back(moduleInfo);
    plugin->SendEvent(E_PLUGINLOAD);
    plugin->Load();
    return true;
}
#endif

void Player::HandleScriptReloadStarted(StringHash eventType, VariantMap& eventData)
{
#ifdef URHO3D_ANGELSCRIPT
    if (scriptFile_->GetFunction("void Stop()"))
        scriptFile_->Execute("void Stop()");
#endif
}

void Player::HandleScriptReloadFinished(StringHash eventType, VariantMap& eventData)
{
#ifdef URHO3D_ANGELSCRIPT
    // Restart the script application after reload
    if (!scriptFile_->Execute("void Start()"))
    {
        scriptFile_.Reset();
        ErrorExit();
    }
#endif
}

void Player::HandleScriptReloadFailed(StringHash eventType, VariantMap& eventData)
{
#ifdef URHO3D_ANGELSCRIPT
    scriptFile_.Reset();
    ErrorExit();
#endif
}

void Player::GetScriptFileName()
{
    const ea::vector<ea::string>& arguments = GetArguments();
    if (arguments.size() && arguments[0][0] != '-') {
        scriptFileName_ = GetInternalPath(arguments[0]);
    } else {
#ifdef SERVER
    scriptFileName_ = "LuaScripts/Server.lua";
#else
    scriptFileName_ = "LuaScripts/Lobby.lua";
#endif
    }
}

/// A simple player loader.
class PlayerHost : public Player
{
    URHO3D_OBJECT(PlayerHost, Player);
public:
    /// Construct.
    explicit PlayerHost(Context* context) : Player(context) { }
    /// Extend initialization of player application.
    void Start() override
    {
#if URHO3D_PLUGINS && URHO3D_SAMPLES && URHO3D_STATIC
        // Static plugins must be initialized manually.
        // SubscribeToEvent(E_REGISTERSTATICPLUGINS, [this](StringHash, VariantMap&) {
        //     RegisterPlugin(new GamePlugin(context_));
        //     UnsubscribeFromEvent(E_REGISTERSTATICPLUGINS);
        // });
#endif
        BaseClassName::Start();
    }
};

} // namespace Urho3D

namespace
{

	class ExampleHelloWorld : public entry::AppI
	{
	public:
		ExampleHelloWorld(const char* _name, const char* _description, const char* _url)
			: entry::AppI(_name, _description, _url)
		{
		}

		void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
		{
			//Args args(_argc, _argv);

			m_width = _width;
			m_height = _height;
			// m_debug = BGFX_DEBUG_TEXT;
			m_reset = BGFX_RESET_VSYNC;

			bgfx::Init init;
			init.type = bgfx::RendererType::Count;// args.m_type;//bgfx::RendererType::OpenGL;
			init.vendorId = BGFX_PCI_ID_NONE;// args.m_pciId;
			init.resolution.width = m_width;
			init.resolution.height = m_height;
			init.resolution.reset = m_reset;
			bgfx::init(init);

			// Enable debug text.
			// bgfx::setDebug(m_debug);

			// Set view 0 clear state.
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL
				, 0x303030ff
				, 1.0f
				, 0
			);

			//imguiCreate();
		}

		virtual int shutdown() override
		{
			//imguiDestroy();

			// Shutdown bgfx.
			bgfx::shutdown();

			return 0;
		}

		bool update() override
		{
            static bool first_time = true;
            if (first_time) {
                context_ = std::make_shared<Urho3D::Context>();
                //urho3D_player_ = std::make_unique<Urho3D::SamplesManager>(context_.get());
                urho3D_player_ = std::make_unique<Urho3D::PlayerHost>(context_.get());
                urho3D_player_->Init();
                // static_scene_ = std::make_unique<StaticScene>(context_.get());
                // static_scene_->Init();
                first_time = false;
            }
            urho3D_player_->RunFrame();

			// if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
			// {
			// 	imguiBeginFrame(m_mouseState.m_mx
			// 		, m_mouseState.m_my
			// 		, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
			// 		| (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
			// 		| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
			// 		, m_mouseState.m_mz
			// 		, uint16_t(m_width)
			// 		, uint16_t(m_height)
			// 	);

			// 	showExampleDialog(this);

			// 	imguiEndFrame();

			// 	// Set view 0 default viewport.
			// 	bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));

			// 	// This dummy draw call is here to make sure that view 0 is cleared
			// 	// if no other draw calls are submitted to view 0.
            // bgfx::touch(0);

			// 	// Use debug font to print information about this example.
			// 	bgfx::dbgTextClear();
			// 	bgfx::dbgTextImage(
			// 		bx::max<uint16_t>(uint16_t(m_width / 2 / 8), 20) - 20
			// 		, bx::max<uint16_t>(uint16_t(m_height / 2 / 16), 6) - 6
			// 		, 40
			// 		, 12
			// 		, s_logo
			// 		, 160
			// 	);
			// 	bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");

			// 	bgfx::dbgTextPrintf(80, 1, 0x0f, "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    \x1b[; 7m    \x1b[0m");
			// 	bgfx::dbgTextPrintf(80, 2, 0x0f, "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    \x1b[;15m    \x1b[0m");

			// 	const bgfx::Stats* stats = bgfx::getStats();
			// 	bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters."
			// 		, stats->width
			// 		, stats->height
			// 		, stats->textWidth
			// 		, stats->textHeight
			// 	);

				// Advance to next frame. Rendering thread will be kicked to
				// process submitted rendering primitives.
			//	bgfx::frame();

				return true;
			// }

			// return false;
		}

		entry::MouseState m_mouseState;

		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_debug;
		uint32_t m_reset;
        std::shared_ptr<Urho3D::Context> context_;
        std::unique_ptr<Urho3D::PlayerHost> urho3D_player_;
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	ExampleHelloWorld
	, "00-helloworld"
	, "Initialization and debug text."
	, "https://bkaradzic.github.io/bgfx/examples.html#helloworld"
);
