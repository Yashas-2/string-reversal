/* ============================================================
   Parallel String Reversal Using OpenMP — front-end logic
   ============================================================ */

// --- DOM references ---
const inputText    = document.getElementById('input-text');
const charCount    = document.getElementById('char-count');
const seqBtn       = document.getElementById('seq-btn');
const parBtn       = document.getElementById('par-btn');
const errorBox     = document.getElementById('error-box');

const originalOut  = document.getElementById('original-out');
const modeBadge    = document.getElementById('mode-badge');
const timeOut      = document.getElementById('time-out');
const correctOut   = document.getElementById('correct-out');

const comparisonCard = document.getElementById('comparison-card');
const compareSeq     = document.getElementById('compare-seq');
const comparePar     = document.getElementById('compare-par');
const compareThreads = document.getElementById('compare-threads');
const compareSpeedup = document.getElementById('compare-speedup');

// --- stored results for the comparison section ---
let seqResult = null;
let parResult = null;

// --- thread selector ---
let selectedThreads = 4;

document.querySelectorAll('.thread-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.thread-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    selectedThreads = Number(btn.dataset.threads);
  });
});

// --- character counter ---
function updateCharCount() {
  charCount.textContent = inputText.value.length;
}
inputText.addEventListener('input', updateCharCount);
updateCharCount();

// --- helpers ---
function formatTime(seconds) {
  if (seconds === null || seconds === undefined) return '—';
  if (seconds === 0) return '0.000000 s';
  if (seconds < 0.001) return seconds.toExponential(2) + ' s';
  return seconds.toFixed(6) + ' s';
}

function showError(message) {
  errorBox.textContent = message;
  errorBox.hidden = false;
}

function clearError() {
  errorBox.hidden = true;
}

// --- call the Flask API ---
async function callReverse(mode) {
  clearError();

  const text = inputText.value;
  if (!text.trim()) {
    showError('Please enter a string to reverse.');
    return null;
  }

  const body = { text, mode };
  if (mode === 'parallel') body.threads = selectedThreads;

  seqBtn.disabled = true;
  parBtn.disabled = true;

  try {
    const response = await fetch('/api/reverse', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
    const data = await response.json();
    if (!response.ok) {
      showError(data.error || 'The request failed.');
      return null;
    }
    return data;
  } catch {
    showError('Could not connect to the backend. Is Flask running?');
    return null;
  } finally {
    seqBtn.disabled = false;
    parBtn.disabled = false;
  }
}

// --- display a single result ---
function showResult(data) {
  originalOut.textContent = data.original || '(empty)';

  // Animated reversed output
  animateReversed(data.reversed || '');

  modeBadge.textContent = data.mode === 'parallel'
    ? `⚡ ${data.threads} threads`
    : '1 thread';

  timeOut.textContent = formatTime(data.time_seconds);

  if (data.correct) {
    correctOut.textContent = '✓ Correct';
    correctOut.className   = 'stat-value correct';
  } else {
    correctOut.textContent = '✗ Incorrect';
    correctOut.className   = 'stat-value incorrect';
  }
}

// --- fly-in letter animation ---
function animateReversed(text) {
  const container = document.getElementById('reversed-out');
  container.innerHTML = '';

  if (!text) {
    const span = document.createElement('span');
    span.className = 'placeholder';
    span.textContent = '(empty)';
    container.appendChild(span);
    return;
  }

  // Skip animation for very long strings to keep the page responsive
  if (text.length > 300) {
    container.textContent = text;
    return;
  }

  const chars = Array.from(text);
  chars.forEach((ch, i) => {
    const span = document.createElement('span');
    span.className = ch === ' ' ? 'fly-char space' : 'fly-char';
    span.textContent = ch === ' ' ? '\u00A0' : ch;

    // Stagger: each letter waits 30ms after the previous one
    span.style.animationDelay = (i * 30) + 'ms';

    // Random start position: random angle + distance
    const angle  = Math.random() * Math.PI * 2;
    const dist   = 50 + Math.random() * 70;
    const rotate = (Math.random() - 0.5) * 140;
    const scale  = 0.3 + Math.random() * 0.5;

    span.style.setProperty('--fly-x', (Math.cos(angle) * dist).toFixed(1) + 'px');
    span.style.setProperty('--fly-y', (Math.sin(angle) * dist).toFixed(1) + 'px');
    span.style.setProperty('--fly-r', rotate.toFixed(1) + 'deg');
    span.style.setProperty('--fly-s', scale.toFixed(2));

    container.appendChild(span);
  });
}

// --- update comparison card ---
function updateComparison() {
  if (!seqResult || !parResult) {
    comparisonCard.hidden = true;
    return;
  }

  compareSeq.textContent     = formatTime(seqResult.time_seconds);
  comparePar.textContent     = formatTime(parResult.time_seconds);
  compareThreads.textContent = String(parResult.threads);

  if (parResult.time_seconds > 0) {
    const speedup = seqResult.time_seconds / parResult.time_seconds;
    compareSpeedup.textContent = speedup.toFixed(2) + ' ×';
  } else {
    compareSpeedup.textContent = 'n/a';
  }

  comparisonCard.hidden = false;
}

// --- button handlers ---
seqBtn.addEventListener('click', async () => {
  const data = await callReverse('sequential');
  if (!data) return;
  seqResult = data;
  showResult(data);
  updateComparison();
});

parBtn.addEventListener('click', async () => {
  const data = await callReverse('parallel');
  if (!data) return;
  parResult = data;
  showResult(data);
  updateComparison();
});
