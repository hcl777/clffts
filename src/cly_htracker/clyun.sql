-- MySQL dump 10.13  Distrib 5.1.73, for redhat-linux-gnu (i386)
--
-- Host: localhost    Database: clyun
-- ------------------------------------------------------
-- Server version	5.5.27

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `delete_log`
--

DROP TABLE IF EXISTS `delete_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `delete_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `hash` varchar(64) NOT NULL,
  `peer_name` varchar(32) DEFAULT NULL,
  `ftype` int(11) DEFAULT NULL,
  `del_type` int(11) DEFAULT NULL,
  `name` varchar(128) DEFAULT NULL,
  `create_type` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `error`
--

DROP TABLE IF EXISTS `error`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `error` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `peer_name` varchar(64) DEFAULT NULL,
  `err` int(11) NOT NULL,
  `systemver` varchar(32) DEFAULT NULL,
  `appname` varchar(32) DEFAULT NULL,
  `appver` varchar(32) DEFAULT NULL,
  `description` varchar(128) DEFAULT NULL,
  `create_time` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `file`
--

DROP TABLE IF EXISTS `file`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `file` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '文件的id,自动分配',
  `hash` varchar(64) NOT NULL COMMENT '文件的32字节值',
  `size` bigint(20) DEFAULT NULL COMMENT '文件大小',
  `name` varchar(128) DEFAULT NULL COMMENT '文件名，含相对路径',
  `subhash` varchar(1024) DEFAULT NULL COMMENT '子块hash',
  `flag` int(11) DEFAULT NULL COMMENT '用于指定是否有效,无效的不支持搜索',
  `extmsg` varchar(128) DEFAULT NULL COMMENT '用于第三方关联信息',
  `create_time` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `netflow`
--

DROP TABLE IF EXISTS `netflow`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `netflow` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `file_id` int(11) DEFAULT NULL,
  `peer_id` int(11) DEFAULT NULL,
  `group_id` int(11) DEFAULT '0',
  `peer_type` int(11) DEFAULT NULL,
  `file_size` bigint(20) DEFAULT NULL,
  `rcvB0` bigint(20) DEFAULT NULL,
  `rcvB1` bigint(20) DEFAULT NULL,
  `rcvB2` bigint(20) DEFAULT NULL,
  `rcvB3` bigint(20) DEFAULT NULL,
  `rcvB4` bigint(20) DEFAULT NULL,
  `create_time` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `peer`
--

DROP TABLE IF EXISTS `peer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `peer` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'peer_id',
  `peer_name` varchar(32) NOT NULL COMMENT 'mac+apppath',
  `alias` varchar(64) DEFAULT NULL COMMENT '名称标识',
  `peer_type` int(11) NOT NULL DEFAULT '2' COMMENT '1server,2client',
  `used` int(11) NOT NULL DEFAULT '0' COMMENT '是否激活',
  `group_id` int(11) DEFAULT '0',
  `appver` varchar(32) DEFAULT NULL COMMENT '软件版本',
  `addr` varchar(128) DEFAULT NULL COMMENT 'ip:port:nattype',
  `nattype` int(11) DEFAULT '6' COMMENT '网关类型',
  `downing_hashs` varchar(64) DEFAULT NULL COMMENT '当前正在下载的所有文件',
  `ff_num` int(11) DEFAULT '0' COMMENT '用户的已有文件数',
  `ff_checksum` bigint(21) DEFAULT '0' COMMENT '完整文件的checksum校验',
  `task_update_id` int(11) DEFAULT '0' COMMENT '标记任务变更，每次加1，不与task表的task_id对应。',
  `sync_peer_ids` varchar(128) DEFAULT '0' COMMENT '指定同步源，多个同步源以，号分隔。',
  `alive_time` datetime DEFAULT NULL COMMENT '最后在线时间',
  `create_time` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `source`
--

DROP TABLE IF EXISTS `source`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `source` (
  `id` int(20) NOT NULL AUTO_INCREMENT COMMENT '方便查询更新',
  `file_id` int(11) DEFAULT NULL COMMENT '对应file表中的id',
  `peer_id` int(11) DEFAULT NULL COMMENT '对应peer表中的id',
  `ftype` int(11) DEFAULT '2' COMMENT '指示文件下载类型，是分发下载还是直接共享等。',
  `progress` int(11) DEFAULT '0' COMMENT '下载完成进度，1000为完整',
  `state` int(11) NOT NULL DEFAULT '1' COMMENT '1下载，2暂停，3停止，9删除',
  `priority` int(11) NOT NULL DEFAULT '50' COMMENT '下载优先级，数值越大越低',
  `speed` int(11) NOT NULL DEFAULT '0' COMMENT '下载速度，KBps',
  `fails` int(11) NOT NULL DEFAULT '0' COMMENT '下载失败次数',
  `task_id` int(11) NOT NULL DEFAULT '0' COMMENT '0表示不是分发，非0表示是分发任务的',
  `task_date` date DEFAULT NULL COMMENT '分发时间',
  `modify_time` datetime DEFAULT NULL,
  `create_time` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `task`
--

DROP TABLE IF EXISTS `task`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `task` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(64) DEFAULT NULL,
  `peer_ids` text,
  `file_ids` text,
  `create_time` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2017-12-22 14:05:37
