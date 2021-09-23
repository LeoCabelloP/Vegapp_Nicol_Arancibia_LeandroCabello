import { Component, OnInit } from '@angular/core';
import { MenuController } from '@ionic/angular';

interface Componente{
  icon: string;
  name: string;
  redirecTo:string;

}

@Component({
  selector: 'app-inicio',
  templateUrl: './inicio.page.html',
  styleUrls: ['./inicio.page.scss'],
})
export class InicioPage implements OnInit {

  componentes : Componente[] =[
    {
      icon: 'home-outline',
      name: 'Inicio', 
      redirecTo: '/inicio'
    },
    {
      icon: 'cart-outline', 
      name: 'Tienda', 
      redirecTo: '/page2'
    },
    {
      icon: 'person-add-outline', 
      name: 'Registro', 
      redirecTo: '/page3'
    },
  ]
  
  constructor(private menuController: MenuController) { }

  ngOnInit() {
  }

  mostrarMenu(){
    this.menuController.open('first');
  }

}