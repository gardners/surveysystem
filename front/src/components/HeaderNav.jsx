//Basic Navbar made with bootstrap
import React, { Component } from 'react';
import { Link } from 'react-router-dom';

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

class Navbar extends Component {
    render() {
        return (
            <header>
                <nav className="navbar navbar-expand-md navbar-dark fixed-top bg-dark shadow-sm">
                    <span className="navbar-brand align-items-center">Survey Project</span>

                    <button className="navbar-toggler" type="button" data-toggle="collapse" aria-controls="navbarCollapse" aria-expanded="false" aria-label="Toggle navigation"
                    onClick={ () => toggle(document.getElementById('header-nav--collapse')) }>
                        <span className="navbar-toggler-icon"></span>
                    </button>

                    <div className="collapse navbar-collapse" id="header-nav--collapse">
                        <ul className="navbar-nav mr-auto">
                            <li className={ (/^\/surveys/.test(this.props.location.pathname)) ? 'nav-item active' : 'nav-item' }>
                                <Link className="nav-link" to="/surveys">Survey List</Link>
                            </li>
                        </ul>

                        { /*
                        <form className="form-inline mt-2 mt-md-0">
                            <input className="form-control mr-sm-2" type="text" placeholder="Search" aria-label="Search" />
                            <button className="btn btn-outline-success my-2 my-sm-0" type="submit">Search</button>
                        </form>
                        */ }
                    </div>
                </nav>
            </header>
        );
    }
}

Navbar.propTypes = {};

export default Navbar;
