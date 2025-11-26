import 'react-mosaic-component/react-mosaic-component.css';
import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { Mosaic, MosaicWindow, MosaicNode } from 'react-mosaic-component';
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
    direction: 'column',
    first: 'graph',
    second: {
      direction: 'row',
      first: 'console',
      second: 'inspector',
      splitPercentage: 60,
    },
    splitPercentage: 70,
  },
  splitPercentage: 20,
};

const createDefaultTree = (): MosaicNode<PanelId> => JSON.parse(JSON.stringify(DEFAULT_TREE)) as MosaicNode<PanelId>;

const LAYOUT_STORAGE_PREFIX = 'vortex.editor.layout';

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
            <MosaicWindow<PanelId> path={path} createNode={() => 'graph'} title={TITLE[id]}>
              <div className="w-full h-full bg-ui-panel border border-ui-border rounded-lg overflow-hidden">{RENDER(id)}</div>
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
