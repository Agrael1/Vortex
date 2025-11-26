import React from 'react';
import { describe, expect, it, beforeEach, afterEach } from 'vitest';
import { render, fireEvent, cleanup } from '@testing-library/react';
import { RecoilRoot, useRecoilState } from 'recoil';
import { autosavePreferenceAtom, AUTOSAVE_STORAGE_KEY } from '../preferences';

afterEach(() => {
  cleanup();
});

const AutosaveProbe = () => {
  const [enabled, setEnabled] = useRecoilState(autosavePreferenceAtom);
  return (
    <button data-testid="autosave-toggle" onClick={() => setEnabled((prev) => !prev)}>
      {enabled ? 'on' : 'off'}
    </button>
  );
};

describe('autosavePreferenceAtom', () => {
  beforeEach(() => {
    window.localStorage.clear();
  });

  it('hydrates from localStorage and persists updates', () => {
    window.localStorage.setItem(AUTOSAVE_STORAGE_KEY, 'false');

    const { getByTestId } = render(
      <RecoilRoot>
        <AutosaveProbe />
      </RecoilRoot>,
    );

    const button = getByTestId('autosave-toggle');
    expect(button.textContent).toBe('off');

    fireEvent.click(button);

    expect(button.textContent).toBe('on');
    expect(window.localStorage.getItem(AUTOSAVE_STORAGE_KEY)).toBe('true');
  });
});
