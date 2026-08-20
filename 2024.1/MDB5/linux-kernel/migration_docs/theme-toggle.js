// Theme toggle — shared localStorage key with software docs for consistency
(function() {
    var MOON = '<svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor"><path d="M17.293 13.293A8 8 0 016.707 2.707a8.001 8.001 0 1010.586 10.586z"/></svg>';
    var SUN  = '<svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2.25a.75.75 0 01.75.75v2.25a.75.75 0 01-1.5 0V3a.75.75 0 01.75-.75zM7.5 12a4.5 4.5 0 119 0 4.5 4.5 0 01-9 0zM18.894 6.166a.75.75 0 00-1.06-1.06l-1.591 1.59a.75.75 0 101.06 1.061l1.591-1.59zM21.75 12a.75.75 0 01-.75.75h-2.25a.75.75 0 010-1.5H21a.75.75 0 01.75.75zM17.834 18.894a.75.75 0 001.06-1.06l-1.59-1.591a.75.75 0 10-1.061 1.06l1.59 1.591zM12 18a.75.75 0 01.75.75V21a.75.75 0 01-1.5 0v-2.25A.75.75 0 0112 18zM7.758 17.303a.75.75 0 00-1.061-1.06l-1.591 1.59a.75.75 0 001.06 1.061l1.591-1.59zM6 12a.75.75 0 01-.75.75H3a.75.75 0 010-1.5h2.25A.75.75 0 016 12zM6.697 7.757a.75.75 0 001.06-1.06l-1.59-1.591a.75.75 0 00-1.061 1.06l1.59 1.591z"/></svg>';

    function getTheme() {
        return localStorage.getItem('theme') || 'dark'; // default dark
    }

    function setTheme(theme) {
        document.documentElement.setAttribute('data-theme', theme);
        localStorage.setItem('theme', theme);
        var btn = document.querySelector('.theme-toggle');
        if (!btn) return;
        if (theme === 'dark') {
            btn.innerHTML = MOON;
            btn.setAttribute('aria-label', 'Switch to light theme');
        } else {
            btn.innerHTML = SUN;
            btn.setAttribute('aria-label', 'Switch to dark theme');
        }
    }

    function init() {
        var btn = document.createElement('div');
        btn.className = 'theme-toggle';
        btn.setAttribute('role', 'button');
        btn.setAttribute('tabindex', '0');
        document.body.appendChild(btn);

        setTheme(getTheme());

        btn.addEventListener('click', function() {
            setTheme(getTheme() === 'dark' ? 'light' : 'dark');
        });
        btn.addEventListener('keydown', function(e) {
            if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); btn.click(); }
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
