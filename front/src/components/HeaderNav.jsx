//Basic HeaderNav made with bootstrap
import React, { useContext } from 'react';
import { NavLink } from 'react-router-dom';

import { AuthContext } from '../Context';
import { DropdownMenu, MenuLink } from './bootstrap/DropdownMenu';

import logo from '../assets/logo.png';

// toggles navbar collapse
// element.classList support is ie > 10, we need this to work always!
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

const User = function() {
    const auth = useContext(AuthContext);

    let name = 'Login';
    let cls = 'btn btn-danger';

    if (!auth.protected) {
        return(null);
    }

    if (auth.user) {
        name = (auth.user.name) ? auth.user.name : auth.user.email;
        cls = 'btn btn-success';
    }

    return(
        <NavLink className={ cls } activeClassName="btn-light" to="/login">
            { name }
        </NavLink>
    );
};

User.propTypes = {};

const HeaderNav = function({ location }) {

    return (
        <header>
            <nav className="navbar navbar-expand-md navbar-dark fixed-top bg-dark shadow-sm">
                <NavLink to="/" className="navbar-brand align-items-center"><img src={ logo } className="mr-3" title={ surveyProvider } alt="logo" height="30" />{ siteName }</NavLink>

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
                    <ul className="navbar-nav mr-auto" />

                    <ul className="navbar-nav mt-2 mt-md-0">
                        <li className="nav-item">
                            <NavLink className="btn btn-secondary" activeClassName="btn-light" to="/surveys">Surveys</NavLink>
                        </li>
                        {
                            (process.env.NODE_ENV !== 'production') ?
                                <li className="nav-item ml-3">
                                    <DropdownMenu title="Demos" buttonClassName={ (/^\/demo/.test(location.pathname)) ? 'btn btn-light' : 'btn btn-secondary' }>
                                        <MenuLink to="/demo/form">Form Elements</MenuLink>
                                        <MenuLink to="/demo/analyse">Analysis</MenuLink>
                                        <MenuLink to="/demo/manifest">Manifest</MenuLink>
                                    </DropdownMenu>
                                </li>
                            : null
                        }
                        <li className="nav-item ml-3">
                            <User/>
                        </li>
                    </ul>
                </div>
            </nav>
        </header>
    );
};

HeaderNav.propTypes = {};

export default HeaderNav;
