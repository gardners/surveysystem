//Basic HeaderNav made with bootstrap
import React, { useContext } from 'react';
import { Link } from 'react-router-dom';

import { AuthContext } from '../Context';
import { DropdownMenu, MenuLink } from './bootstrap/DropdownMenu';
import OAuth2 from '../OAuth2';

import logo from '../assets/logo.png';

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

const {
    REACT_APP_SURVEY_PROVIDER,
    REACT_APP_SITE_NAME,
} = process.env;

const surveyProvider = REACT_APP_SURVEY_PROVIDER.trim();
const siteName = REACT_APP_SITE_NAME.trim();


const LoginLink = function() {
    const auth = useContext(AuthContext);

    if (auth.user) {
        return (
            <Link className="btn btn-success" to="/login"><i className="fas fa-user"></i> { auth.user.replace(/(.{9})..+/, "$1...") }</Link>
        );
    }

    return(
        <a className="btn btn-danger" href={ OAuth2.loginUrl() }>Login</a>
    );
};

LoginLink.propTypes = {};

const HeaderNav = function({ location }) {

    return (
        <header>
            <nav className="navbar navbar-expand-md navbar-dark fixed-top bg-dark shadow-sm">
                <Link to="/" className="navbar-brand align-items-center"><img src={ logo } className="mr-3" title={ surveyProvider } alt="logo" height="30" />{ siteName }</Link>

                <button
                    className="navbar-toggler"
                    type="button" data-toggle="collapse" aria-controls="navbarCollapse" aria-expanded="false" aria-label="Toggle navigation"
                    onClick={ (e) => {
                        e.preventDefault();
                        toggle(document.getElementById('header-nav--collapse'));
                    } }
                >
                    <span className="navbar-toggler-icon"></span>
                </button>

                <div className="collapse navbar-collapse" id="header-nav--collapse">
                    <ul className="navbar-nav mr-auto">

                    </ul>

                    <ul className="navbar-nav mt-2 mt-md-0">
                        <li className="nav-item">
                            <Link className={ (/^\/surveys/.test(location.pathname)) ? 'btn btn-light' : 'btn btn-secondary' } to="/surveys">Surveys</Link>
                        </li>
                        {
                            (process.env.NODE_ENV !== 'production') ?
                                <li className="nav-item ml-3">
                                    <DropdownMenu title="Demos">
                                        <MenuLink to="/demo/form">Form Elements</MenuLink>
                                        <MenuLink to="/demo/analyse">Analysis</MenuLink>
                                        <MenuLink to="/demo/manifest">Manifest</MenuLink>
                                    </DropdownMenu>
                                </li>
                            : null
                        }
                        <li className="nav-item ml-3">
                            <LoginLink />
                        </li>
                    </ul>
                </div>
            </nav>
        </header>
    );
};

HeaderNav.propTypes = {};

export default HeaderNav;
