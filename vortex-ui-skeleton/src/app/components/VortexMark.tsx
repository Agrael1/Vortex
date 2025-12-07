import type { SVGProps } from 'react';

export function VortexMark({ className = '', ...props }: SVGProps<SVGSVGElement>) {
  return (
    <svg
      viewBox="0 0 512 512"
      xmlns="http://www.w3.org/2000/svg"
      aria-hidden="true"
      className={className}
      {...props}
    >
      <defs>
        <linearGradient id="header_mark_paint0" x1="107" y1="61.001" x2="282.581" y2="207.611" gradientUnits="userSpaceOnUse">
          <stop stopColor="#8FD3FF" />
          <stop offset="0.55" stopColor="#7AA7FF" />
          <stop offset="1" stopColor="#A78BFA" />
        </linearGradient>
        <linearGradient id="header_mark_paint1" x1="350.374" y1="29.463" x2="311.196" y2="254.825" gradientUnits="userSpaceOnUse">
          <stop stopColor="#8FD3FF" />
          <stop offset="0.55" stopColor="#7AA7FF" />
          <stop offset="1" stopColor="#A78BFA" />
        </linearGradient>
        <linearGradient id="header_mark_paint2" x1="499.374" y1="224.462" x2="284.616" y2="303.214" gradientUnits="userSpaceOnUse">
          <stop stopColor="#8FD3FF" />
          <stop offset="0.55" stopColor="#7AA7FF" />
          <stop offset="1" stopColor="#A78BFA" />
        </linearGradient>
        <linearGradient id="header_mark_paint3" x1="405" y1="451" x2="229.419" y2="304.389" gradientUnits="userSpaceOnUse">
          <stop stopColor="#8FD3FF" />
          <stop offset="0.55" stopColor="#7AA7FF" />
          <stop offset="1" stopColor="#A78BFA" />
        </linearGradient>
        <linearGradient id="header_mark_paint4" x1="161.626" y1="482.537" x2="200.804" y2="257.175" gradientUnits="userSpaceOnUse">
          <stop stopColor="#8FD3FF" />
          <stop offset="0.55" stopColor="#7AA7FF" />
          <stop offset="1" stopColor="#A78BFA" />
        </linearGradient>
        <linearGradient id="header_mark_paint5" x1="12.626" y1="287.538" x2="227.384" y2="208.786" gradientUnits="userSpaceOnUse">
          <stop stopColor="#8FD3FF" />
          <stop offset="0.55" stopColor="#7AA7FF" />
          <stop offset="1" stopColor="#A78BFA" />
        </linearGradient>
      </defs>
      <g opacity="0.95">
        <path
          d="M256 108C245 108 234 110 225 114C190 129 167 165 167 204C167 213 168 221 170 229C172 236 167 241 161 239C139 232 121 215 112 193C109 185 107 176 107 167C107 98.0001 174 48.0001 244 64.0001C250 66.0001 253 73.0001 250 78.0001C244 94.0001 230 108 210 108H256Z"
          fill="url(#header_mark_paint0)"
        />
        <path
          d="M384.172 182C378.672 172.474 371.44 163.947 363.476 158.153C332.985 135.342 290.308 133.424 256.533 152.924C248.739 157.424 242.311 162.29 236.383 168.022C231.321 173.254 224.49 171.424 223.222 165.228C218.285 142.675 224.007 118.587 238.56 99.7924C243.988 93.1943 250.782 86.9623 258.576 82.4623C318.332 47.9623 395.133 80.986 416.277 149.608C417.545 155.804 412.983 161.902 407.153 161.804C390.296 164.608 371.172 159.483 361.172 142.163L384.172 182Z"
          fill="url(#header_mark_paint1)"
        />
        <path
          d="M384.172 330C389.672 320.474 393.44 309.948 394.476 300.153C398.985 262.342 379.308 224.424 345.533 204.924C337.739 200.424 330.311 197.29 322.383 195.022C315.321 193.254 313.49 186.424 318.222 182.228C335.285 166.675 359.007 159.587 382.56 162.792C390.988 164.194 399.782 166.962 407.576 171.462C467.332 205.962 477.133 288.986 428.277 341.608C423.545 345.804 415.983 344.902 413.153 339.804C402.296 326.608 397.172 307.483 407.172 290.163L384.172 330Z"
          fill="url(#header_mark_paint2)"
        />
        <path
          d="M256 404C267 404 278 402 287 398C322 383 345 347 345 308C345 299 344 291 342 283C340 276 345 271 351 273C373 280 391 297 400 319C403 327 405 336 405 345C405 414 338 464 268 448C262 446 259 439 262 434C268 418 282 404 302 404H256Z"
          fill="url(#header_mark_paint3)"
        />
        <path
          d="M127.828 330C133.328 339.526 140.56 348.053 148.524 353.847C179.015 376.658 221.692 378.576 255.467 359.076C263.261 354.576 269.689 349.71 275.617 343.978C280.679 338.746 287.51 340.576 288.778 346.772C293.715 369.325 287.993 393.414 273.44 412.208C268.012 418.806 261.218 425.038 253.424 429.538C193.668 464.038 116.867 431.014 95.7231 362.392C94.4551 356.196 99.0173 350.098 104.847 350.196C121.704 347.392 140.828 352.517 150.828 369.837L127.828 330Z"
          fill="url(#header_mark_paint4)"
        />
        <path
          d="M127.828 182C122.328 191.526 118.56 202.053 117.524 211.847C113.015 249.658 132.692 287.576 166.467 307.076C174.261 311.576 181.689 314.71 189.617 316.978C196.679 318.746 198.51 325.576 193.778 329.772C176.715 345.325 152.993 352.413 129.44 349.208C121.012 347.806 112.218 345.038 104.424 340.538C44.668 306.038 34.8667 223.014 83.7231 170.392C88.4551 166.196 96.0173 167.098 98.8475 172.196C109.704 185.392 114.828 204.517 104.828 221.837L127.828 182Z"
          fill="url(#header_mark_paint5)"
        />
      </g>
    </svg>
  );
}
