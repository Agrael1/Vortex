import 'react-mosaic-component/react-mosaic-component.css';
import React, { useCallback, useContext, useEffect, useMemo, useRef, useState } from 'react';
import { Mosaic, MosaicWindow, MosaicNode, MosaicContext, MosaicWindowContext, getAndAssertNodeAtPathExists } from 'react-mosaic-component';
import { GraphPanel } from '../panels/GraphPanel';
import { InspectorPanel } from '../panels/InspectorPanel';
import { NodeLibraryPanel } from '../panels/NodeLibraryPanel';
import { ConsolePanel } from '../panels/ConsolePanel';
import { AppHeader } from '../components/AppHeader';
import { engine, type LastProject } from '@/app/services/ipc/cefBridge';
import { useRecoilState, useRecoilValue, useSetRecoilState } from 'recoil';
import { projectPathAtom } from '@state/atoms/project';
import { selectedNodePtrAtom } from '@state/atoms/editor';
import { useEngineNodeUpdates } from '@state/hooks/useEngineNodeUpdates';
import { useEngineEdgeUpdates } from '@state/hooks/useEngineEdgeUpdates';
import { autosavePreferenceAtom } from '@state/atoms/preferences';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { usePersistenceActions } from '@state/hooks/usePersistenceActions';

type PanelId = 'graph' | 'inspector' | 'library' | 'console';

const DEFAULT_TREE: MosaicNode<PanelId> = {
  direction: 'row',
  first: 'library',
  second: {
    direction: 'row',
    first: {
      direction: 'column',
      first: 'graph',
      second: 'console',
      splitPercentage: 78,
    },
    second: 'inspector',
    splitPercentage: 76,
  },
  splitPercentage: 18,
};

const createDefaultTree = (): MosaicNode<PanelId> => JSON.parse(JSON.stringify(DEFAULT_TREE)) as MosaicNode<PanelId>;

type ControlVariant = 'replace' | 'split' | 'expand' | 'close';

const ReplaceIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M7 6h10l-3-3" />
    <path d="M17 18H7l3 3" />
    <path d="M7 6v4" />
    <path d="M17 18v-4" />
  </svg>
);

const SplitIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <rect x="4" y="5" width="6" height="14" rx="1.5" />
    <rect x="14" y="5" width="6" height="14" rx="1.5" />
    <path d="M12 5v14" />
  </svg>
);

const ExpandIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M8 5H5v3" />
    <path d="M16 5h3v3" />
    <path d="M8 19H5v-3" />
    <path d="M16 19h3v-3" />
  </svg>
);

const CloseIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M7 7l10 10" />
    <path d="M17 7L7 17" />
  </svg>
);

const ICON_COMPONENTS: Record<ControlVariant, React.FC> = {
  replace: ReplaceIcon,
  split: SplitIcon,
  expand: ExpandIcon,
  close: CloseIcon,
};

const isPromiseLike = (value: unknown): value is PromiseLike<void> =>
  typeof value === 'object' && value !== null && typeof (value as PromiseLike<void>).then === 'function';

interface ToolbarButtonProps {
  variant: ControlVariant;
  label: string;
  onClick?: () => void;
  disabled?: boolean;
}

const MosaicToolbarButton = ({ variant, label, onClick, disabled }: ToolbarButtonProps) => {
  const Icon = ICON_COMPONENTS[variant];
  const isDisabled = disabled || !onClick;

  return (
    <button
      type="button"
      className={`mosaic-default-control mosaic-default-control-${variant}`}
      aria-label={label}
      onClick={
        isDisabled
          ? undefined
          : (event) => {
              event.stopPropagation();
              onClick();
            }
      }
      disabled={isDisabled}
    >
      <span className="vortex-mosaic-icon" aria-hidden="true">
        <Icon />
      </span>
    </button>
  );
};

const MosaicToolbarControls = ({ canCreate }: { canCreate: boolean }) => {
  const mosaicContext = useContext(MosaicContext);
  const windowContext = useContext(MosaicWindowContext);
  const mosaicActions = mosaicContext?.mosaicActions;
  const windowActions = windowContext?.mosaicWindowActions;
  const path = windowActions?.getPath?.();

  const guardAndRun = useCallback(
    (label: string, action?: () => void | Promise<void>) => {
      if (!mosaicActions || !action || !path) {
        return undefined;
      }

      return () => {
        const root = mosaicActions.getRoot();
        try {
          getAndAssertNodeAtPathExists(root, path);
        } catch (error) {
          console.warn(`[DockLayout] Skipped "${label}" because the layout path is stale.`, error);
          return;
        }

        try {
          const result = action();
          if (isPromiseLike(result)) {
            Promise.resolve(result).catch((err) => console.error(`[DockLayout] ${label} failed`, err));
          }
        } catch (error) {
          console.error(`[DockLayout] ${label} failed`, error);
        }
      };
    },
    [mosaicActions, path],
  );

  const replaceAction = windowActions?.replaceWithNew ? () => windowActions.replaceWithNew() : undefined;
  const splitAction = windowActions?.split ? () => windowActions.split() : undefined;
  const expandAction = mosaicActions && path ? () => mosaicActions.expand(path) : undefined;
  const closeAction = mosaicActions && path ? () => mosaicActions.remove(path) : undefined;

  const handleReplace = guardAndRun('Replace panel', replaceAction);
  const handleSplit = guardAndRun('Split panel', splitAction);
  const handleExpand = guardAndRun('Expand panel', expandAction);
  const handleClose = guardAndRun('Close panel', closeAction);

  const buttons: ToolbarButtonProps[] = [
    { variant: 'replace', label: 'Replace', onClick: handleReplace, disabled: !canCreate },
    { variant: 'split', label: 'Split', onClick: handleSplit, disabled: !canCreate },
    { variant: 'expand', label: 'Expand', onClick: handleExpand },
    { variant: 'close', label: 'Close', onClick: handleClose },
  ];

  return (
    <>
      {buttons.map((button) => (
        <MosaicToolbarButton key={button.variant} {...button} />
      ))}
    </>
  );
};

const LAYOUT_STORAGE_PREFIX = 'vortex.editor.layout.v2';

const getLayoutStorageKey = (projectPath: string | null | undefined) =>
  `${LAYOUT_STORAGE_PREFIX}:${projectPath && projectPath.length ? projectPath : 'default'}`;

const loadLayout = (projectPath: string | null | undefined): MosaicNode<PanelId> | null => {
  if (typeof window === 'undefined') return null;
  try {
    const stored = window.localStorage.getItem(getLayoutStorageKey(projectPath));
    if (!stored) return null;
    const parsed = JSON.parse(stored);
    if (!parsed || typeof parsed !== 'object') return null;
    return parsed as MosaicNode<PanelId>;
  } catch (error) {
    console.warn('[DockLayout] Failed to load layout from storage', error);
    return null;
  }
};

export function DockLayout() {
  const setProjectPath = useSetRecoilState(projectPathAtom);
  const projectPath = useRecoilValue(projectPathAtom);
  const latestProjectPathRef = useRef<string | null>(null);
  useEffect(() => {
    latestProjectPathRef.current = projectPath;
  }, [projectPath]);
  const setSelectedPtr = useSetRecoilState(selectedNodePtrAtom);
  const [autosaveEnabled, setAutosaveEnabled] = useRecoilState(autosavePreferenceAtom);
  const persistenceStatus = useRecoilValue(persistenceStatusAtom);
  const { reloadFromDisk, saveNow } = usePersistenceActions();
  useEngineNodeUpdates();
  useEngineEdgeUpdates();
  const layoutKey = projectPath ?? 'default';
  const [tree, setTree] = useState<MosaicNode<PanelId> | null>(() => loadLayout(layoutKey) ?? createDefaultTree());

  const TITLE: Record<PanelId, string> = {
    graph: 'Graph',
    inspector: 'Inspector',
    library: 'Node Library',
    console: 'Console',
  };

  const handleResetLayout = useCallback(() => {
    setTree(createDefaultTree());
  }, []);

  useEffect(() => {
    const lastKnown = engine.getLastProject()?.path ?? null;
    if (!lastKnown || lastKnown === projectPath) {
      return;
    }
    setProjectPath(lastKnown);
  }, [projectPath, setProjectPath]);

  useEffect(() => {
    const off = engine.on('lastProject:updated', (payload) => {
      const detail = payload as LastProject | null;
      const nextPath = detail?.path ?? null;
      if (nextPath === latestProjectPathRef.current) {
        return;
      }
      latestProjectPathRef.current = nextPath;
      setProjectPath(nextPath);
    });

    return () => {
      off?.();
    };
  }, [setProjectPath]);

  useEffect(() => {
    const stored = loadLayout(layoutKey);
    setTree(stored ?? createDefaultTree());
    setSelectedPtr((current) => (current === null ? current : null));
  }, [layoutKey, setSelectedPtr]);

  useEffect(() => {
    if (typeof window === 'undefined') return;
    const key = getLayoutStorageKey(layoutKey);

    if (!tree) {
      window.localStorage.removeItem(key);
      return;
    }

    try {
      window.localStorage.setItem(key, JSON.stringify(tree));
    } catch (error) {
      console.warn('[DockLayout] Failed to persist layout', error);
    }
  }, [layoutKey, tree]);

  const RENDER = useMemo(
    () => (id: PanelId) => {
      switch (id) {
        case 'graph':
          return <GraphPanel />;
        case 'inspector':
          return <InspectorPanel />;
        case 'library':
          return <NodeLibraryPanel />;
        case 'console':
          return <ConsolePanel />;
      }
    },
    [],
  );

  const formatSavedTimestamp = useCallback((value: string) => {
    try {
      return new Date(value).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    } catch {
      return value;
    }
  }, []);

  const statusDescriptor = useMemo(() => {
    if (!persistenceStatus.isHydrated) {
      return {
        label: 'Preparing project state…',
        tone: 'text-gray-400',
        icon: <span className="inline-block h-3 w-3 animate-spin rounded-full border border-current border-t-transparent" aria-hidden="true" />,
      };
    }

    if (persistenceStatus.isReloading) {
      return {
        label: 'Reloading from disk…',
        tone: 'text-gray-300',
        icon: <span className="inline-block h-3 w-3 animate-spin rounded-full border border-current border-t-transparent" aria-hidden="true" />,
      };
    }

    if (persistenceStatus.lastError) {
      return {
        label: persistenceStatus.lastError,
        tone: 'text-red-300',
        icon: <span aria-hidden="true">⚠️</span>,
      };
    }

    if (persistenceStatus.isSaving) {
      return {
        label: 'Saving project…',
        tone: 'text-gray-200',
        icon: <span className="inline-block h-3 w-3 animate-spin rounded-full border border-current border-t-transparent" aria-hidden="true" />,
      };
    }

    if (persistenceStatus.isDirty) {
      return {
        label: 'Unsaved changes',
        tone: 'text-amber-300',
        icon: <span className="inline-block h-2.5 w-2.5 rounded-full bg-amber-300" aria-hidden="true" />,
      };
    }

    if (persistenceStatus.lastSavedAt) {
      return {
        label: `Saved ${formatSavedTimestamp(persistenceStatus.lastSavedAt)}`,
        tone: 'text-gray-300',
        icon: <span className="inline-block h-2.5 w-2.5 rounded-full bg-emerald-300/80" aria-hidden="true" />,
      };
    }

    return {
      label: 'Ready',
      tone: 'text-gray-500',
      icon: <span className="inline-block h-2.5 w-2.5 rounded-full bg-gray-500/60" aria-hidden="true" />,
    };
  }, [formatSavedTimestamp, persistenceStatus]);

  const handleReload = useCallback(() => {
    reloadFromDisk().catch((error) => {
      console.error('[DockLayout] Reload failed', error);
    });
  }, [reloadFromDisk]);

  const handleSaveNow = useCallback(() => {
    saveNow().catch((error) => {
      console.error('[DockLayout] Save failed', error);
    });
  }, [saveNow]);

  return (
    <div className="w-screen h-screen flex flex-col">
      <AppHeader onResetLayout={handleResetLayout} />
      <div className="flex-1">
        <Mosaic<PanelId>
          renderTile={(id, path) => (
            <MosaicWindow<PanelId>
              path={path}
              createNode={() => 'graph'}
              title={TITLE[id]}
              toolbarControls={<MosaicToolbarControls canCreate={true} />}
            >
              <div className="w-full h-full bg-ui-panel border border-ui-border overflow-hidden">{RENDER(id)}</div>
            </MosaicWindow>
          )}
          value={tree}
          onChange={setTree}
          className="h-full"
        />
      </div>
      <footer className="h-12 border-t border-ui-border px-3 text-xs flex items-center justify-between gap-3 bg-black/40">
        <div className={`flex items-center gap-2 ${statusDescriptor.tone}`}>
          {statusDescriptor.icon}
          <span className="truncate" title={persistenceStatus.lastError ?? undefined}>{statusDescriptor.label}</span>
        </div>
        <div className="flex items-center gap-2 text-[11px]">
          <button
            type="button"
            onClick={handleReload}
            disabled={!persistenceStatus.isHydrated || persistenceStatus.isReloading}
            className="rounded border border-ui-border/40 px-2 py-1 text-gray-200 transition hover:border-ui-border disabled:opacity-50"
            title="Reload snapshot from disk"
          >
            {persistenceStatus.isReloading ? 'Reloading…' : 'Reload'}
          </button>
          <button
            type="button"
            onClick={handleSaveNow}
            disabled={!persistenceStatus.isHydrated || persistenceStatus.isSaving}
            className="rounded border border-ui-border/40 px-2 py-1 text-gray-200 transition hover:border-ui-border disabled:opacity-50"
            title="Force save current scene"
          >
            {persistenceStatus.isSaving ? 'Saving…' : 'Save now'}
          </button>
          <button
            type="button"
            onClick={() => setAutosaveEnabled((prev) => !prev)}
            className={`inline-flex items-center gap-1 rounded border px-2 py-1 transition ${
              autosaveEnabled ? 'border-emerald-400/40 text-emerald-200' : 'border-red-400/40 text-red-200'
            }`}
            aria-pressed={autosaveEnabled}
            title="Toggle automatic persistence"
          >
            <span>Autosave</span>
            <span>{autosaveEnabled ? 'ON' : 'OFF'}</span>
          </button>
        </div>
      </footer>
    </div>
  );
}
