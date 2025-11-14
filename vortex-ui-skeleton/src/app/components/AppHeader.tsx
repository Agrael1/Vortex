import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { PlaybackControls } from '@/app/components/PlaybackControls';
import { Vortex } from '@/bridge/vortex';
import { engine, type Recent } from '@/app/services/ipc/cefBridge';

type MenuKey = 'file' | 'edit' | 'view' | 'window' | 'help';

type MenuItem =
  | { type: 'action'; label: string; shortcut?: string; description?: string; disabled?: boolean; onSelect?: () => void }
  | { type: 'separator' }
  | { type: 'heading'; label: string };

interface AppHeaderProps {
  className?: string;
  onResetLayout?: () => void;
}

export function AppHeader({ className = '', onResetLayout }: AppHeaderProps) {
  const navigate = useNavigate();
  const headerRef = useRef<HTMLDivElement | null>(null);
  const menubarRef = useRef<HTMLDivElement | null>(null);
  const menuRefs = useRef<Record<MenuKey, HTMLDivElement | null>>({
    file: null,
    edit: null,
    view: null,
    window: null,
    help: null,
  });
  const [activeMenu, setActiveMenu] = useState<MenuKey | null>(null);
  const [recents, setRecents] = useState<Recent[]>([]);
  const [isPerformingAction, setIsPerformingAction] = useState(false);
  const [isSaving, setIsSaving] = useState(false);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const saveMessageTimerRef = useRef<number | null>(null);

  const closeMenus = useCallback(() => {
    setActiveMenu(null);
  }, []);

  const handleToggleMenu = useCallback((key: MenuKey) => {
    setActiveMenu((current) => (current === key ? null : key));
  }, []);

  const handleMenuHover = useCallback(
    (key: MenuKey) => {
      setActiveMenu((current) => (current ? key : current));
    },
    []
  );

  useEffect(() => {
    if (!activeMenu) return;

    const handlePointer = (event: PointerEvent) => {
      const targetNode = event.target as Node;
      const menuElement = activeMenu ? menuRefs.current[activeMenu] : null;

      if (menuElement && menuElement.contains(targetNode)) {
        return;
      }

      if (menubarRef.current && menubarRef.current.contains(targetNode)) {
        return;
      }

      setActiveMenu(null);
    };

    const handleKey = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        event.preventDefault();
        setActiveMenu(null);
      }
    };

    document.addEventListener('pointerdown', handlePointer);
    document.addEventListener('keydown', handleKey);
    return () => {
      document.removeEventListener('pointerdown', handlePointer);
      document.removeEventListener('keydown', handleKey);
    };
  }, [activeMenu]);

  useEffect(() => {
    let cancelled = false;

    const loadRecents = async () => {
      try {
        const list = await engine.listRecent();
        if (!cancelled) {
          setRecents(list);
        }
      } catch (error) {
        console.warn('[AppHeader] Failed to load recent projects', error);
      }
    };

    loadRecents();
    const off = engine.on('recents:updated', (payload) => {
      if (Array.isArray(payload) && !cancelled) {
        setRecents(payload as Recent[]);
      }
    });

    return () => {
      cancelled = true;
      off?.();
    };
  }, []);

  const quickTestPattern = useCallback(async () => {
    closeMenus();
    try {
      console.log('[Quick Test] Starting Image→Window test pattern...');

      const add = (window as any).__GraphPanelAddNode as (t: string, x?: number, y?: number) => Promise<number>;
      if (!add) {
        console.error('[Quick Test] __GraphPanelAddNode not available');
        return;
      }

      console.log('[Quick Test] Creating ImageInput node...');
      const imagePtr = await add('ImageInput', 200, 200);
      console.log('[Quick Test] ImageInput created with ptr:', imagePtr);

      console.log('[Quick Test] Creating WindowOutput node...');
      const windowPtr = await add('WindowOutput', 520, 220);
      console.log('[Quick Test] WindowOutput created with ptr:', windowPtr);

      console.log('[Quick Test] Connecting nodes...', imagePtr, '->', windowPtr);
      await Vortex.connect(imagePtr, 0, windowPtr, 0);
      console.log('[Quick Test] Image→Window pipeline created successfully!');
      console.log('[Quick Test] Now click Play to see test pattern');
    } catch (error) {
      console.error('[Quick Test] Error creating Image→Window:', error);
    }
  }, [closeMenus]);

  const quickStreamToWindow = useCallback(async () => {
    closeMenus();
    try {
      console.log('[Quick] Starting Stream→Window creation...');

      const add = (window as any).__GraphPanelAddNode as (t: string, x?: number, y?: number) => Promise<number>;
      if (!add) {
        console.error('[Quick] __GraphPanelAddNode not available');
        return;
      }

      console.log('[Quick] Creating StreamInput node...');
      const streamPtr = await add('StreamInput', 200, 200);
      console.log('[Quick] StreamInput created with ptr:', streamPtr);

      console.log('[Quick] Creating WindowOutput node...');
      const windowPtr = await add('WindowOutput', 520, 220);
      console.log('[Quick] WindowOutput created with ptr:', windowPtr);

      console.log('[Quick] Connecting nodes...', streamPtr, '->', windowPtr);
      await Vortex.connect(streamPtr, 0, windowPtr, 0);
      console.log('[Quick] Stream→Window pipeline created successfully!');
    } catch (error) {
      console.error('[Quick] Error creating Stream→Window:', error);
    }
  }, [closeMenus]);

  const handleNavigateHub = useCallback(() => {
    closeMenus();
    navigate('/hub');
  }, [closeMenus, navigate]);

  const handleNewProject = useCallback(() => {
    closeMenus();
    navigate('/hub', { state: { action: 'new-project' } });
  }, [closeMenus, navigate]);

  const handleOpenProjectPicker = useCallback(async () => {
    closeMenus();
    setIsPerformingAction(true);
    try {
      const selected = await engine.browseForProject();
      if (!selected) return;
      await engine.openProject(selected);
    } catch (error) {
      console.error('[AppHeader] Unable to open project from picker', error);
    } finally {
      setIsPerformingAction(false);
    }
  }, [closeMenus]);

  const handleOpenRecent = useCallback(async (entry: Recent) => {
    closeMenus();
    setIsPerformingAction(true);
    try {
      await engine.openProject(entry.path);
    } catch (error) {
      console.error('[AppHeader] Unable to open recent project', entry.path, error);
    } finally {
      setIsPerformingAction(false);
    }
  }, [closeMenus]);

  const handleSaveProject = useCallback(async () => {
    closeMenus();
    if (isSaving) return;

    setIsSaving(true);
    setSaveMessage(null);

    try {
      const status = await engine.saveProject();
      const timestamp = new Date().toLocaleTimeString();
      setSaveMessage(
        status === 'fallback'
          ? `Saved locally (${timestamp})`
          : `Saved ${timestamp}`
      );
    } catch (error) {
      console.error('[AppHeader] Unable to save project', error);
      setSaveMessage('Save failed');
    } finally {
      setIsSaving(false);
    }
  }, [closeMenus, isSaving]);

  const handleResetLayout = useCallback(() => {
    closeMenus();
    onResetLayout?.();
  }, [closeMenus, onResetLayout]);

  const handleShowShortcuts = useCallback(() => {
    closeMenus();
    window.alert('Shortcuts\n\nCtrl+N  New Project\nCtrl+O  Open Project\nSpace    Play/Stop');
  }, [closeMenus]);

  const handleOpenDocs = useCallback(() => {
    closeMenus();
    window.alert('Documentation is bundled with the app. Check the docs/ directory for guides.');
  }, [closeMenus]);

  const handleAbout = useCallback(() => {
    closeMenus();
    window.alert('Vortex — Video Compositor\nVersion 1.0.0-alpha');
  }, [closeMenus]);

  const fileMenuItems = useMemo<MenuItem[]>(() => {
    const items: MenuItem[] = [
      { type: 'action', label: 'New Project…', shortcut: 'Ctrl+N', disabled: isPerformingAction, onSelect: handleNewProject },
      { type: 'action', label: 'Open Project…', shortcut: 'Ctrl+O', disabled: isPerformingAction, onSelect: handleOpenProjectPicker },
      { type: 'action', label: 'Save Project', shortcut: 'Ctrl+S', disabled: isSaving, onSelect: handleSaveProject },
      { type: 'action', label: 'Open Hub', shortcut: 'Ctrl+H', onSelect: handleNavigateHub },
      { type: 'separator' },
    ];

    if (recents.length > 0) {
      items.push({ type: 'heading', label: 'Recent Projects' });
      recents.slice(0, 8).forEach((entry) => {
        items.push({
          type: 'action',
          label: entry.name,
          description: entry.path,
          disabled: isPerformingAction,
          onSelect: () => handleOpenRecent(entry),
        });
      });
    } else {
      items.push({ type: 'action', label: 'No recent projects', disabled: true });
    }

    return items;
  }, [handleNavigateHub, handleNewProject, handleOpenProjectPicker, handleOpenRecent, handleSaveProject, isPerformingAction, isSaving, recents]);

  const viewMenuItems = useMemo<MenuItem[]>(() => {
    return [
      { type: 'action', label: 'Reset Layout', shortcut: 'Ctrl+0', disabled: !onResetLayout, onSelect: handleResetLayout },
    ];
  }, [handleResetLayout, onResetLayout]);

  const editMenuItems = useMemo<MenuItem[]>(() => {
    return [
      { type: 'action', label: 'Undo', shortcut: 'Ctrl+Z', disabled: true },
      { type: 'action', label: 'Redo', shortcut: 'Ctrl+Y', disabled: true },
    ];
  }, []);

  useEffect(() => {
    if (!saveMessage) return;

    if (saveMessageTimerRef.current) {
      window.clearTimeout(saveMessageTimerRef.current);
    }

    saveMessageTimerRef.current = window.setTimeout(() => {
      setSaveMessage(null);
      saveMessageTimerRef.current = null;
    }, 4000);

    return () => {
      if (saveMessageTimerRef.current) {
        window.clearTimeout(saveMessageTimerRef.current);
        saveMessageTimerRef.current = null;
      }
    };
  }, [saveMessage]);

  useEffect(() => {
    return () => {
      if (saveMessageTimerRef.current) {
        window.clearTimeout(saveMessageTimerRef.current);
        saveMessageTimerRef.current = null;
      }
    };
  }, []);

  const windowMenuItems = useMemo<MenuItem[]>(() => {
    return [
      { type: 'action', label: 'Focus Graph', disabled: true },
      { type: 'action', label: 'Toggle Console', disabled: true },
    ];
  }, []);

  const helpMenuItems = useMemo<MenuItem[]>(() => {
    return [
      { type: 'action', label: 'Keyboard Shortcuts', onSelect: handleShowShortcuts },
      { type: 'action', label: 'Documentation', onSelect: handleOpenDocs },
      { type: 'separator' },
      { type: 'action', label: 'About Vortex', onSelect: handleAbout },
    ];
  }, [handleAbout, handleOpenDocs, handleShowShortcuts]);

  const menus = useMemo(() => ({
    file: fileMenuItems,
    edit: editMenuItems,
    view: viewMenuItems,
    window: windowMenuItems,
    help: helpMenuItems,
  }), [editMenuItems, fileMenuItems, helpMenuItems, viewMenuItems, windowMenuItems]);

  const renderMenu = (key: MenuKey) => {
    if (activeMenu !== key) return null;

    const items = menus[key];

    return (
      <div
        ref={(element) => {
          menuRefs.current[key] = element;
        }}
        className="absolute left-0 top-full mt-1 min-w-[200px] rounded-md border border-ui-border bg-ui-panel shadow-xl z-20"
      >
        {items.map((item, index) => {
          if (item.type === 'separator') {
            return <div key={`sep-${index}`} className="my-1 h-px bg-ui-border/70" />;
          }

          if (item.type === 'heading') {
            return (
              <div key={`heading-${item.label}`} className="px-3 py-1 text-xs uppercase tracking-wide text-gray-500">
                {item.label}
              </div>
            );
          }

          const action = item as Extract<MenuItem, { type: 'action' }>;
          return (
            <button
              key={`item-${action.label}-${index}`}
              type="button"
              disabled={action.disabled}
              onClick={action.onSelect}
              className={`w-full px-3 py-1.5 text-left text-sm flex items-center justify-between gap-4 transition-colors ${
                action.disabled ? 'text-gray-500 cursor-not-allowed' : 'text-gray-200 hover:bg-white/10'
              }`}
            >
              <span className="flex-1 overflow-hidden">
                <span className="block truncate">{action.label}</span>
                {action.description ? (
                  <span className="block text-xs text-gray-500 truncate">{action.description}</span>
                ) : null}
              </span>
              {action.shortcut ? <span className="text-xs text-gray-500">{action.shortcut}</span> : null}
            </button>
          );
        })}
      </div>
    );
  };

  return (
    <header
      ref={headerRef}
      className={`h-12 border-b border-ui-border bg-ui-panel flex items-center justify-between px-4 ${className}`}
    >
      <div className="flex items-center gap-6">
        <div className="flex items-center gap-3">
          <strong className="text-lg">Vortex</strong>
          <div className="w-px h-4 bg-gray-600" />
          <span className="text-sm text-gray-400">Video Compositor</span>
        </div>

        <nav ref={menubarRef} className="flex items-center gap-2 text-sm text-gray-400">
          {(['file', 'edit', 'view', 'window', 'help'] as MenuKey[]).map((key) => (
            <div key={key} className="relative">
              <button
                type="button"
                onClick={() => handleToggleMenu(key)}
                onMouseEnter={() => handleMenuHover(key)}
                className={`px-2 py-1 rounded hover:text-gray-100 ${activeMenu === key ? 'bg-white/10 text-gray-100' : ''}`}
              >
                {key.charAt(0).toUpperCase() + key.slice(1)}
              </button>
              {renderMenu(key)}
            </div>
          ))}
        </nav>
      </div>

      <PlaybackControls />

      <div className="flex items-center gap-2">
        <div className="flex items-center gap-2 mr-4">
          <button
            onClick={quickTestPattern}
            className="px-3 py-1 text-xs rounded-md bg-blue-600/20 text-blue-400 border border-blue-600/30 hover:bg-blue-600/30 transition-colors"
            title="Create Image→Window test pattern"
          >
            Test: Image
          </button>

          <button
            onClick={quickStreamToWindow}
            className="px-3 py-1 text-xs rounded-md bg-purple-600/20 text-purple-400 border border-purple-600/30 hover:bg-purple-600/30 transition-colors"
            title="Create Stream→Window pipeline"
          >
            Test: Stream
          </button>
        </div>

        <div className="flex items-center gap-3 text-xs text-gray-500">
          {isSaving ? (
            <span className="flex items-center gap-2 text-gray-300">
              <span className="inline-block h-3 w-3 animate-spin rounded-full border border-gray-400 border-t-transparent" />
              <span>Saving…</span>
            </span>
          ) : saveMessage ? (
            <span className="text-gray-300">{saveMessage}</span>
          ) : null}
          <span>v1.0.0-alpha</span>
        </div>
      </div>
    </header>
  );
}