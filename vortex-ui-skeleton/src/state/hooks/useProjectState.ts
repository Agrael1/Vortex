import { useRecoilValue } from 'recoil';
import { projectPathAtom, projectMetaAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';

export function useProjectState() {
  const path = useRecoilValue(projectPathAtom);
  const meta = useRecoilValue(projectMetaAtom);
  const settings = useRecoilValue(projectSettingsAtom);
  const graph = useRecoilValue(graphSnapshotAtom);

  return { path, meta, settings, graph };
}
