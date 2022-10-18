//Basic Header made with bootstrap
import React, { useContext } from 'react';
import { NavLink, withRouter } from 'react-router-dom';

import { AuthContext } from './AuthProvider';
import { DropdownMenu, MenuLink } from './bootstrap/DropdownMenu';

import logo from '../assets/logo.png';


const {
    NODE_ENV,
    REACT_APP_SURVEY_PROVIDER,
    REACT_APP_SITE_NAME,
} = process.env;

const surveyProvider = REACT_APP_SURVEY_PROVIDER.trim();
const siteName = REACT_APP_SITE_NAME.trim();


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


const User = function() {
    const auth = useContext(AuthContext);

    if (!auth.is_protected) {
        return(null);
    }

    if (!auth.user) {
        return (
            <NavLink className="btn btn-danger" to="/login">
                <i className="fa fa-user"></i>
            </NavLink>
        );
    }

    return (
        <NavLink className="btn btn-success" to="/login">
            <i className="fa fa-user"></i>
        </NavLink>
    );

/*
    const username = auth.username || '[UNKNOWN]';
    const logout = function(e) {
        e.preventDefault();
        auth.logout();
    };

    return(
        <DropdownMenu
            title={ <i className="fa fa-user"></i> }
            buttonClass="btn btn-success"
            alignmentClass="dropdown-menu-right"
        >
            <div className="dropdown-item-text">Hello <strong>{ username }</strong></div>
            <div className="dropdown-divider"></div>
            <div className="dropdown-item-text"><button className="btn btn-sm btn-block btn-light" onClick={ logout } >Logout</button></div>
        </DropdownMenu>
    );
*/

};

const DemoMenu = function() {
    const buttonClass = (/^\/demo/.test(window.location.pathname)) ? 'btn btn-light' : 'btn btn-secondary';

    return (
        <DropdownMenu
            title="Demos"
            buttonClass={ buttonClass }
            alignmentClass="dropdown-menu-right"
        >
            <MenuLink to="/demo/form">Form Elements</MenuLink>
            <MenuLink to="/demo/analyse">Analysis</MenuLink>
            <MenuLink to="/demo/manifest">Manifest</MenuLink>
        </DropdownMenu>
    );
};

User.propTypes = {};

const Header = function({ location }) {

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
                        { (NODE_ENV !== 'production') && <li className="nav-item ml-3"><DemoMenu /></li> }
                        <li className="nav-item ml-3">
                            <User/>
                        </li>
                    </ul>
                </div>
            </nav>
        </header>
    );
};

Header.propTypes = {};

export default withRouter(Header);
