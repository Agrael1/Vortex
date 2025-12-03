import { useCallback, useEffect, useLayoutEffect, useMemo, useState } from 'react';
import './IntroOverlay.css';

const INTRO_DURATION = 6400;
const LETTERS = 'VORTEX';
const LETTER_DELAY_STEP = 0.1;
const INTRO_ACTIVE_CLASS = 'vortex-intro-active';

type IntroFlags = {
  faviconIn: boolean;
  faviconOut: boolean;
  logoIn: boolean;
  glowOn: boolean;
  textIn: boolean;
  logoSpin: boolean;
  logoBreath: boolean;
  logoFloat: boolean;
};

const DEFAULT_FLAGS: IntroFlags = {
  faviconIn: false,
  faviconOut: false,
  logoIn: false,
  glowOn: false,
  textIn: false,
  logoSpin: false,
  logoBreath: false,
  logoFloat: false,
};

export function IntroOverlay() {
  const initialDone = useMemo(() => {
    if (typeof window === 'undefined') {
      return false;
    }
    return Boolean((window as unknown as { __VortexIntroDone?: boolean }).__VortexIntroDone);
  }, []);

  const [isDone, setIsDone] = useState(initialDone);
  const [flags, setFlags] = useState<IntroFlags>(() =>
    initialDone ? { ...DEFAULT_FLAGS, textIn: true, logoIn: true, glowOn: true, logoBreath: true, logoFloat: true } : DEFAULT_FLAGS,
  );
  const letters = useMemo(() => LETTERS.split(''), []);

  const markIntroComplete = useCallback(() => {
    setIsDone(true);
    if (typeof window !== 'undefined') {
      (window as unknown as { __VortexIntroDone?: boolean }).__VortexIntroDone = true;
      window.dispatchEvent(new CustomEvent('vortex:intro:done'));
    }
  }, []);

  const updateFlags = useCallback((patch: Partial<IntroFlags>) => {
    setFlags((prev) => ({ ...prev, ...patch }));
  }, []);

  useLayoutEffect(() => {
    if (typeof document === 'undefined' || initialDone) {
      return;
    }

    const body = document.body;
    const html = document.documentElement;
    if (!body || !html) {
      return;
    }

    const prevBodyOverflow = body.style.overflow;
    const prevHtmlOverflow = html.style.overflow;
    const shouldLock = !isDone;

    if (shouldLock) {
      body.style.overflow = 'hidden';
      html.style.overflow = 'hidden';
      body.classList.add(INTRO_ACTIVE_CLASS);
    } else {
      body.classList.remove(INTRO_ACTIVE_CLASS);
    }

    return () => {
      body.style.overflow = prevBodyOverflow;
      html.style.overflow = prevHtmlOverflow;
      body.classList.remove(INTRO_ACTIVE_CLASS);
    };
  }, [initialDone, isDone]);

  useEffect(() => {
    if (isDone || initialDone) {
      return;
    }

    const isJsDom = typeof navigator !== 'undefined' && /jsdom/i.test(navigator.userAgent ?? '');
    if (isJsDom) {
      markIntroComplete();
      return;
    }

    const timers: number[] = [];
    timers.push(window.setTimeout(() => updateFlags({ faviconIn: true }), 200));
    timers.push(window.setTimeout(() => updateFlags({ faviconOut: true }), 1600));
    timers.push(
      window.setTimeout(() => {
        updateFlags({ logoIn: true, glowOn: true, logoSpin: true });
      }, 2200),
    );
    timers.push(
      window.setTimeout(() => {
        updateFlags({ logoSpin: false, logoBreath: true, logoFloat: true });
      }, 3800),
    );
    timers.push(window.setTimeout(() => updateFlags({ textIn: true }), 2600));
    timers.push(window.setTimeout(markIntroComplete, INTRO_DURATION));

    return () => {
      timers.forEach((id) => window.clearTimeout(id));
    };
  }, [initialDone, isDone, markIntroComplete, updateFlags]);

  if (isDone) {
    return null;
  }

  const innerClass = [
    'intro-inner',
    flags.faviconIn ? 'play-favicon-in' : '',
    flags.faviconOut ? 'play-favicon-out' : '',
    flags.logoIn ? 'play-logo-in' : '',
    flags.textIn ? 'play-text-in' : '',
    flags.glowOn ? 'glow-on' : '',
  ]
    .filter(Boolean)
    .join(' ');

  const logoClass = [
    'intro-icon',
    'logo',
    flags.logoSpin ? 'spin-phase' : '',
    flags.logoBreath ? 'breath-phase' : '',
    flags.logoFloat ? 'float-phase' : '',
  ]
    .filter(Boolean)
    .join(' ');

  return (
    <div className="vortex-intro intro" aria-hidden="true">
      <div className={innerClass}>
        <div className="intro-icon-wrapper">
          <div className="intro-icon favicon">
            <svg width="512" height="512" viewBox="0 0 512 512" fill="none" xmlns="http://www.w3.org/2000/svg">
              <path d="M256 508C395.176 508 508 395.176 508 256C508 116.824 395.176 4 256 4C116.824 4 4 116.824 4 256C4 395.176 116.824 508 256 508Z" fill="#0A0F16" />
              <path d="M256 108C228 108 202 132 194 160C183 196 193 228 212 253C216 258 213 265 207 264C182 260 157 246 141 223C117 203 110 233 117 260C132 318 193 357 256 344C262 343 265 350 262 355C251 373 231 386 207 386C297 386 369 313 369 224C369 135 346 108 256 108Z" fill="url(#favicon_paint0)" />
              <path d="M384.172 182C370.172 157.751 336.387 147.235 308.138 154.306C271.462 162.78 248.749 187.44 236.598 216.395C234.268 222.359 226.706 223.261 224.572 217.565C215.536 193.914 215.16 165.263 227.079 139.907C232.399 109.122 202.919 118.06 183.036 137.622C140.306 179.613 137.031 251.94 179.79 300C183.656 304.696 179.094 310.794 173.263 310.696C152.175 310.17 130.917 299.349 118.917 278.565C163.917 356.507 263.137 382.361 340.213 337.861C417.289 293.361 429.172 259.942 384.172 182Z" fill="url(#favicon_paint1)" />
              <path d="M384.172 330C398.172 305.751 390.387 271.235 370.138 250.306C344.462 222.78 311.749 215.44 280.598 219.395C274.268 220.359 269.706 214.261 273.572 209.565C289.536 189.914 314.16 175.263 342.079 172.907C371.399 162.122 348.919 141.06 322.036 133.622C264.306 117.613 200.031 150.94 179.79 212C177.656 217.696 170.094 216.794 167.263 211.696C157.175 193.17 155.917 169.349 167.917 148.565C122.917 226.507 150.137 325.361 227.213 369.861C304.289 414.361 339.172 407.942 384.172 330Z" fill="url(#favicon_paint2)" />
              <path d="M256 404C284 404 310 380 318 352C329 316 319 284 300 259C296 254 299 247 305 248C330 252 355 266 371 289C395 309 402 279 395 252C380 194 319 155 256 168C250 169 247 162 250 157C261 139 281 126 305 126C215 126 143 199 143 288C143 377 166 404 256 404Z" fill="url(#favicon_paint3)" />
              <path d="M127.828 330C141.828 354.249 175.613 364.765 203.862 357.694C240.538 349.22 263.251 324.56 275.402 295.605C277.732 289.641 285.294 288.739 287.428 294.435C296.464 318.086 296.84 346.737 284.921 372.093C279.601 402.878 309.081 393.94 328.964 374.378C371.694 332.387 374.969 260.06 332.21 212C328.344 207.304 332.906 201.206 338.737 201.304C359.825 201.83 381.083 212.651 393.083 233.435C348.083 155.493 248.863 129.639 171.787 174.139C94.7109 218.639 82.8282 252.058 127.828 330Z" fill="url(#favicon_paint4)" />
              <path d="M127.828 182C113.828 206.249 121.613 240.765 141.862 261.694C167.538 289.22 200.251 296.56 231.402 292.605C237.732 291.641 242.294 297.739 238.428 302.435C222.464 322.086 197.84 336.737 169.921 339.093C140.601 349.878 163.081 370.94 189.964 378.378C247.694 394.387 311.969 361.06 332.21 300C334.344 294.304 341.906 295.206 344.737 300.304C354.825 318.83 356.083 342.651 344.083 363.435C389.083 285.493 361.863 186.639 284.787 142.139C207.711 97.6392 172.828 104.058 127.828 182Z" fill="url(#favicon_paint5)" />
              <defs>
                <linearGradient id="favicon_paint0" x1="114.42" y1="108" x2="391.347" y2="361.597" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="favicon_paint1" x1="313.382" y1="59.3882" x2="232.224" y2="426.013" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="favicon_paint2" x1="454.962" y1="207.388" x2="96.8767" y2="320.415" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="favicon_paint3" x1="397.58" y1="404" x2="120.653" y2="150.403" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="favicon_paint4" x1="198.618" y1="452.612" x2="279.776" y2="85.9875" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="favicon_paint5" x1="57.0383" y1="304.612" x2="415.123" y2="191.585" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
              </defs>
            </svg>
          </div>
          <div className={logoClass}>
            <svg width="512" height="512" viewBox="0 0 512 512" fill="none" xmlns="http://www.w3.org/2000/svg">
              <path className="petal petal-a" d="M256 108C245 108 234 110 225 114C190 129 167 165 167 204C167 213 168 221 170 229C172 236 167 241 161 239C139 232 121 215 112 193C109 185 107 176 107 167C107 98.0001 174 48.0001 244 64.0001C250 66.0001 253 73.0001 250 78.0001C244 94.0001 230 108 210 108H256Z" fill="url(#logo_paint0)" />
              <path className="petal petal-b" d="M384.172 182C378.672 172.474 371.44 163.947 363.476 158.153C332.985 135.342 290.308 133.424 256.533 152.924C248.739 157.424 242.311 162.29 236.383 168.022C231.321 173.254 224.49 171.424 223.222 165.228C218.285 142.675 224.007 118.587 238.56 99.7924C243.988 93.1943 250.782 86.9623 258.576 82.4623C318.332 47.9623 395.133 80.986 416.277 149.608C417.545 155.804 412.983 161.902 407.153 161.804C390.296 164.608 371.172 159.483 361.172 142.163L384.172 182Z" fill="url(#logo_paint1)" />
              <path className="petal petal-c" d="M384.172 330C389.672 320.474 393.44 309.948 394.476 300.153C398.985 262.342 379.308 224.424 345.533 204.924C337.739 200.424 330.311 197.29 322.383 195.022C315.321 193.254 313.49 186.424 318.222 182.228C335.285 166.675 359.007 159.587 382.56 162.792C390.988 164.194 399.782 166.962 407.576 171.462C467.332 205.962 477.133 288.986 428.277 341.608C423.545 345.804 415.983 344.902 413.153 339.804C402.296 326.608 397.172 307.483 407.172 290.163L384.172 330Z" fill="url(#logo_paint2)" />
              <path className="petal petal-d" d="M256 404C267 404 278 402 287 398C322 383 345 347 345 308C345 299 344 291 342 283C340 276 345 271 351 273C373 280 391 297 400 319C403 327 405 336 405 345C405 414 338 464 268 448C262 446 259 439 262 434C268 418 282 404 302 404H256Z" fill="url(#logo_paint3)" />
              <path className="petal petal-e" d="M127.828 330C133.328 339.526 140.56 348.053 148.524 353.847C179.015 376.658 221.692 378.576 255.467 359.076C263.261 354.576 269.689 349.71 275.617 343.978C280.679 338.746 287.51 340.576 288.778 346.772C293.715 369.325 287.993 393.414 273.44 412.208C268.012 418.806 261.218 425.038 253.424 429.538C193.668 464.038 116.867 431.014 95.7231 362.392C94.4551 356.196 99.0173 350.098 104.847 350.196C121.704 347.392 140.828 352.517 150.828 369.837L127.828 330Z" fill="url(#logo_paint4)" />
              <path className="petal petal-f" d="M127.828 182C122.328 191.526 118.56 202.053 117.524 211.847C113.015 249.658 132.692 287.576 166.467 307.076C174.261 311.576 181.689 314.71 189.617 316.978C196.679 318.746 198.51 325.576 193.778 329.772C176.715 345.325 152.993 352.413 129.44 349.208C121.012 347.806 112.218 345.038 104.424 340.538C44.668 306.038 34.8667 223.014 83.7231 170.392C88.4552 166.196 96.0173 167.098 98.8475 172.196C109.704 185.392 114.828 204.517 104.828 221.837L127.828 182Z" fill="url(#logo_paint5)" />
              <defs>
                <linearGradient id="logo_paint0" x1="107" y1="61.0013" x2="282.581" y2="207.611" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="0.55" stopColor="#7AA7FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="logo_paint1" x1="350.374" y1="29.4629" x2="311.196" y2="254.825" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="0.55" stopColor="#7AA7FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="logo_paint2" x1="499.374" y1="224.462" x2="284.616" y2="303.214" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="0.55" stopColor="#7AA7FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="logo_paint3" x1="405" y1="450.999" x2="229.419" y2="304.389" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="0.55" stopColor="#7AA7FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="logo_paint4" x1="161.626" y1="482.537" x2="200.804" y2="257.175" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="0.55" stopColor="#7AA7FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
                <linearGradient id="logo_paint5" x1="12.6261" y1="287.538" x2="227.384" y2="208.786" gradientUnits="userSpaceOnUse">
                  <stop stopColor="#8FD3FF" />
                  <stop offset="0.55" stopColor="#7AA7FF" />
                  <stop offset="1" stopColor="#A78BFA" />
                </linearGradient>
              </defs>
            </svg>
          </div>
        </div>
        <div className="intro-text">
          {letters.map((letter, index) => (
            <span key={`${letter}-${index}`} style={flags.textIn ? { animationDelay: `${index * LETTER_DELAY_STEP}s` } : undefined}>
              {letter}
            </span>
          ))}
        </div>
      </div>
    </div>
  );
}
