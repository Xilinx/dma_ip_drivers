// Add top header to the page
document.addEventListener('DOMContentLoaded', function() {
    // Create header HTML
    const headerHTML = `
        <div class="top-header">
            <div class="header-logo">
                <a href="https://www.amd.com" class="amd-logo-link" target="_blank"></a>
                <a href="index.html" class="mdb5-dma-logo-link"></a>
                <a class="version-logo-link"></a>
            </div>
            <nav class="header-nav">
                <a href="https://github.com/Xilinx/dma_ip_drivers/tree/master/MDB5">GitHub</a>
                <a href="https://www.amd.com/en/products/adaptive-socs-and-fpgas.html" target="_blank">Products</a>
                <a href="https://www.amd.com/en/corporate/contacts.html" target="_blank">Contact</a>
                <a href="https://www.amd.com/en/corporate.html" target="_blank">About</a>
            </nav>
            <div class="header-version">
                Version 2025.2
            </div>
        </div>
    `;
    
    // Insert header at the beginning of body
    document.body.insertAdjacentHTML('afterbegin', headerHTML);
    
    // Update active navigation based on current page
    // Note: No active states needed for external links
    const navLinks = document.querySelectorAll('.header-nav a');
    
    navLinks.forEach(link => {
        // Remove any existing active classes since all nav items are external
        link.classList.remove('active');
    });

    // Add navigation buttons by leveraging existing Sphinx navigation
    function addNavigationButtons() {
        // Find the existing hidden Sphinx navigation buttons
        const existingFooter = document.querySelector('.rst-footer-buttons');
        if (!existingFooter) return;

        const prevLink = existingFooter.querySelector('a[rel="prev"]');
        const nextLink = existingFooter.querySelector('a[rel="next"]');

        let navHTML = '<div class="page-navigation">';
        
        // Previous button
        if (prevLink) {
            const prevTitle = prevLink.getAttribute('title') || prevLink.textContent.trim();
            const prevUrl = prevLink.getAttribute('href');
            navHTML += `
                <a href="${prevUrl}" class="nav-btn prev">
                    <span class="nav-icon">arrow_back</span>
                    <span>
                        <span class="nav-direction">PREVIOUS</span>
                        <span class="nav-title">${prevTitle}</span>
                    </span>
                </a>
            `;
        } else {
            navHTML += '<div></div>';
        }
        
        // Next button
        if (nextLink) {
            const nextTitle = nextLink.getAttribute('title') || nextLink.textContent.trim();
            const nextUrl = nextLink.getAttribute('href');
            navHTML += `
                <a href="${nextUrl}" class="nav-btn next">
                    <span>
                        <span class="nav-direction">NEXT</span>
                        <span class="nav-title">${nextTitle}</span>
                    </span>
                    <span class="nav-icon">arrow_forward</span>
                </a>
            `;
        }
        
        navHTML += '</div>';

        // Insert navigation before custom footer
        const content = document.querySelector('.rst-content');
        if (content) {
            const footer = content.querySelector('.custom-footer');
            if (footer) {
                footer.insertAdjacentHTML('beforebegin', navHTML);
            } else {
                content.insertAdjacentHTML('beforeend', navHTML);
            }
        }
    }

    // Add custom footer with links
    const footerHTML = `
        <div class="custom-footer">
            © 2025, Advanced Micro Devices, Inc.
        </div>
    `;
    
    // Find the content wrap and add footer
    const contentWrap = document.querySelector('.wy-nav-content-wrap');
    if (contentWrap) {
        contentWrap.insertAdjacentHTML('beforeend', footerHTML);
    }

    // Add navigation buttons after a slight delay to ensure content is loaded
    setTimeout(addNavigationButtons, 100);
});
