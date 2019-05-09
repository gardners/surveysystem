//Basic HeaderNav made with bootstrap
import React from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

import ThemePicker from './ThemePicker';

// toggles navbar collapse
// element.classList support is ie > 10, we ned this to work always!
const toggle = function(el){
    if (!el) {
        return;
    }
    if(el.className.match(/(?:^|\s)show(?!\S)/)) {
        el.className = el.className.replace( /(?:^|\s)show(?!\S)/g , '' );
        return;
    }
    el.className += ' show';
};

const HeaderNav = function({ location, surveys, surveyProvider, siteName}) {
    return (
        <header>
            <nav className="navbar navbar-expand-md navbar-dark fixed-top bg-dark shadow-sm">
                <Link to="/" className="navbar-brand align-items-center">{ siteName }</Link>

                <button
                    className="navbar-toggler"
                    type="button" data-toggle="collapse" aria-controls="navbarCollapse" aria-expanded="false" aria-label="Toggle navigation"
                    onClick={ () => toggle(document.getElementById('header-nav--collapse')) }
                >
                    <span className="navbar-toggler-icon"></span>
                </button>

                <div className="collapse navbar-collapse" id="header-nav--collapse">
                    <ul className="navbar-nav mr-auto">
                    {
                        surveys.length > 1 &&
                            <li className={ (/^\/surveys/.test(location.pathname)) ? 'nav-item active' : 'nav-item' }>
                                <Link className="nav-link" to="/surveys">Survey List</Link>
                            </li>
                    }
                        <li className={ (/^\/demo/.test(location.pathname)) ? 'nav-item active my-2 my-sm-0' : 'nav-item my-2 my-sm-0' }>
                            <Link className="nav-link" to="/demo/form">Demo form</Link>
                        </li>

                        <li className={ (/^\/demo/.test(location.pathname)) ? 'nav-item active my-2 my-sm-0' : 'nav-item my-2 my-sm-0' }>
                            <Link className="nav-link" to="/demo/analyse">Demo analysis</Link>
                        </li>
                    {
                        process.env.NODE_ENV === 'development' &&
                            <li className={ (/^\/demo/.test(location.pathname)) ? 'nav-item active my-2 my-sm-0' : 'nav-item my-2 my-sm-0' }>
                                <Link className="nav-link" to="/demo/manifest">Demo manifest</Link>
                            </li>
                    }
                    </ul>

                    <ul className="navbar-nav mt-2 mt-md-0">
                        <ThemePicker className="nav-item active my-2 my-sm-0" />
                    </ul>
                </div>
            </nav>
        </header>
    );
};

HeaderNav.propTypes = {
    surveys: PropTypes.arrayOf(PropTypes.string).isRequired,
    surveyProvider: PropTypes.string.isRequired,
    siteName: PropTypes.string.isRequired,
};

export default HeaderNav;
