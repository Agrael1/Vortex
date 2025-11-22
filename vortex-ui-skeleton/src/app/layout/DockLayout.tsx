import 'react-mosaic-component/react-mosaic-component.css';
import React, { useCallback, useEffect, useMemo, useState } from 'react';
import { Mosaic, MosaicWindow, MosaicNode } from 'react-mosaic-component';
import { GraphPanel } from '../panels/GraphPanel';
import { InspectorPanel } from '../panels/InspectorPanel';
import { NodeLibraryPanel } from '../panels/NodeLibraryPanel';
import { ConsolePanel } from '../panels/ConsolePanel';
import { AppHeader } from '../components/AppHeader';
import { engine, type LastProject } from '@/app/services/ipc/cefBridge';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { projectPathAtom } from '@state/atoms/project';
import { selectedNodePtrAtom } from '@state/atoms/editor';
import { useEngineNodeUpdates } from '@state/hooks/useEngineNodeUpdates';

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
  const setSelectedPtr = useSetRecoilState(selectedNodePtrAtom);
  useEngineNodeUpdates();
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
    if (lastKnown) {
      setProjectPath(lastKnown);
    }
  }, [setProjectPath]);

  useEffect(() => {
    const off = engine.on('lastProject:updated', (payload) => {
      const detail = payload as LastProject | null;
      setProjectPath(detail?.path ?? null);
    });

    return () => {
      off?.();
    };
  }, [setProjectPath]);

  useEffect(() => {
    const stored = loadLayout(layoutKey);
    setTree(stored ?? createDefaultTree());
    setSelectedPtr(null);
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
      <footer className="h-6 border-t border-ui-border px-2 text-xs flex items-center justify-between opacity-70">
        <div>Status: Ready</div>
        <div>Autosave: ON</div>
      </footer>
    </div>
  );
}
