import { useEffect, MouseEvent } from 'react';
import { createPortal } from 'react-dom';

const PORTAL_TARGET: HTMLElement | null = typeof document !== 'undefined' ? document.body : null;

export type ExitConfirmModalProps = {
  isOpen: boolean;
  hasUnsavedChanges: boolean;
  isProcessing: boolean;
  onCancel: () => void;
  onConfirmExit: () => void;
  onSaveAndExit: () => void;
};

export function ExitConfirmModal({
  isOpen,
  hasUnsavedChanges,
  isProcessing,
  onCancel,
  onConfirmExit,
  onSaveAndExit,
}: ExitConfirmModalProps) {
  useEffect(() => {
    if (!isOpen) return;

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape' && !isProcessing) {
        event.preventDefault();
        onCancel();
      }
    };

    const previousOverflow = document.body.style.overflow;
    document.body.style.overflow = 'hidden';
    document.addEventListener('keydown', handleKeyDown);

    return () => {
      document.body.style.overflow = previousOverflow;
      document.removeEventListener('keydown', handleKeyDown);
    };
  }, [isOpen, isProcessing, onCancel]);

  if (!isOpen || !PORTAL_TARGET) {
    return null;
  }

  const stopPropagation = (event: MouseEvent<HTMLDivElement>) => {
    event.stopPropagation();
  };

  const headline = hasUnsavedChanges ? 'Unsaved changes detected' : 'Exit Vortex?';
  const body = hasUnsavedChanges
    ? 'Your project has pending edits. Save the scene before exiting to avoid losing work.'
    : 'Are you sure you want to close Vortex? Any running nodes will stop.';

  return createPortal(
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 px-4" onClick={() => (!isProcessing ? onCancel() : undefined)}>
      <div
        className="w-full max-w-md rounded-2xl border border-ui-border/70 bg-ui-panel p-6 shadow-2xl"
        role="dialog"
        aria-modal="true"
        onClick={stopPropagation}
      >
        <header className="mb-4 space-y-1">
          <p className="text-sm uppercase tracking-[0.3em] text-gray-500">Confirm exit</p>
          <h2 className="text-2xl font-semibold text-white">{headline}</h2>
        </header>
        <p className="text-sm text-gray-300">{body}</p>

        <div className="mt-6 flex flex-wrap justify-end gap-3">
          <button
            type="button"
            className="rounded-lg border border-ui-border bg-transparent px-4 py-2 text-sm text-gray-200 transition hover:border-gray-500 hover:text-white disabled:opacity-50"
            onClick={onCancel}
            disabled={isProcessing}
          >
            Cancel
          </button>
          {hasUnsavedChanges && (
            <button
              type="button"
              className="inline-flex items-center gap-2 rounded-lg border border-blue-500/60 bg-blue-600/80 px-4 py-2 text-sm font-medium text-white transition hover:bg-blue-500 disabled:opacity-50"
              onClick={onSaveAndExit}
              disabled={isProcessing}
            >
              {isProcessing && <span className="h-4 w-4 animate-spin rounded-full border-2 border-white/80 border-t-transparent" aria-hidden="true" />}
              <span>Save &amp; Exit</span>
            </button>
          )}
          <button
            type="button"
            className="inline-flex items-center gap-2 rounded-lg border border-red-500/60 bg-red-600/80 px-4 py-2 text-sm font-medium text-white transition hover:bg-red-500 disabled:opacity-50"
            onClick={onConfirmExit}
            disabled={isProcessing}
          >
            {isProcessing && (
              <span className="h-4 w-4 animate-spin rounded-full border-2 border-white/80 border-t-transparent" aria-hidden="true" />
            )}
            <span>{hasUnsavedChanges ? 'Exit without saving' : 'Exit'}</span>
          </button>
        </div>
      </div>
    </div>,
    PORTAL_TARGET,
  );
}
